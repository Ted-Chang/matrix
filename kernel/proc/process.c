/*
 * process.c
 */

#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/matrix.h"
#include "matrix/const.h"
#include "matrix/debug.h"
#include "hal/hal.h"
#include "hal/cpu.h"
#include "mm/page.h"
#include "mm/mmu.h"
#include "mm/malloc.h"
#include "proc/process.h"
#include "proc/sched.h"
#include "proc/thread.h"
#include "elf.h"

struct process_create {
	/* Arguments provided by the caller */
	const char **argv;	// Arguments array
	const char **env;	// Environment array

	/* Information used internally by the loader */
	int argc;		// Argument count

	/* Information to return to the caller */
	int status;		// Status code to return from the call
};

static pid_t _next_pid = 1;

/* Tree of all processes */
static struct avl_tree _proc_tree;

/* Process containing all kernel-mode threads */
struct process *_kernel_proc = NULL;

extern uint32_t _initial_esp;

extern uint32_t read_eip();
extern void sched_init();

static pid_t id_alloc()
{
	return _next_pid++;
}

static void move_stack(uint32_t new_stack, uint32_t size)
{
	uint32_t i, pd_addr;
	uint32_t old_esp, old_ebp;
	uint32_t new_esp, new_ebp;
	uint32_t offset;

	/* Allocate some space for the new stack */
	for (i = new_stack; i >= (new_stack - size); i -= PAGE_SIZE) {
		/* General purpose stack is in user-mode */
		struct page *page;
		
		page = mmu_get_page(_current_mmu_ctx, i, TRUE, 0);
		/* Associate the pte with a physical page */
		page_alloc(page, FALSE, TRUE);
	}

	/* Flush the TLB by reading and writing the page directory address again */
	asm volatile("mov %%cr3, %0" : "=r"(pd_addr));
	asm volatile("mov %0, %%cr3" :: "r"(pd_addr));

	/* Old ESP and EBP, read from registers */
	asm volatile("mov %%esp, %0" : "=r"(old_esp));
	asm volatile("mov %%ebp, %0" : "=r"(old_ebp));

	offset = (uint32_t)new_stack - _initial_esp;
	
	/* Initialize the new ESP and EBP */
	new_esp = old_esp + offset;
	new_ebp = old_ebp + offset;

	DEBUG(DL_DBG, ("move_stack: move stack to new address.\n"
		       "- old_esp(0x%x), old_ebp(0x%x)\n"
		       "- new_esp(0x%x), new_ebp(0x%x)\n"
		       "- _initial_esp(0x%x)\n",
		       old_esp, old_ebp, new_esp, new_ebp, _initial_esp));
	
	/* Copy the old stack to new stack. Although we have switched to cloned
	 * page directory, the kernel memory map was not changed. So we can just
	 * copy the data between the initial esp and the old esp to our new stack.
	 */
	memcpy((void *)new_esp, (void *)old_esp, _initial_esp - old_esp);

	/* Backtrace through the original stack, copying new values into
	 * the new stack
	 */
	for (i = (uint32_t)new_stack; i > ((uint32_t)new_stack - size); i -= 4) {
		uint32_t tmp = *(uint32_t *)i;

		/* If the value of tmp is inside the range of the old stack, assume
		 * it is a base pointer and remap it. This will unfortunately remap
		 * ANY value in this range, whether they are base pointers or not.
		 */
		if ((old_esp < tmp) && (tmp < _initial_esp)) {
			uint32_t *tmp2;
			
			tmp = tmp + offset;
			tmp2 = (uint32_t *)i;
			*tmp2 = tmp;
		}
	}

	/* Change to the new stack */
	asm volatile("mov %0, %%esp" :: "r"(new_esp));
	asm volatile("mov %0, %%ebp" :: "r"(new_ebp));
}

/**
 * Construct a process
 */
static void process_ctor(void *obj, struct process *parent, struct mmu_ctx *ctx)
{
	struct process *p;

	p = (struct process *)obj;
	LIST_INIT(&p->link);
	p->ref_count = 1;
	p->mmu_ctx = ctx;
	p->id = id_alloc();		// Allocate an ID for the process
	p->uid = 500;			// FixMe: set it to the currently logged user
	p->gid = 500;			// FixMe: set it to the current user's group
	p->priority = 16;		// Default priority
	p->max_priority = 31;		// Max priority for the process
	p->quantum = P_QUANTUM;
	p->ticks_left = 0;		// Initial ticks is 0
	p->name[0] = '\0';

	/* Initialize the architecture specific fields */
	p->arch.esp = 0;
	p->arch.ebp = 0;
	p->arch.eip = 0;

	if (parent) {
		/* If parent is NULL, then it must be the init process. We only allocate
		 * kernel stack for non-init process.
		 */
		p->arch.kstack = (uint32_t)kmalloc(KSTACK_SIZE, MM_ALIGN) + KSTACK_SIZE;
		memset((void *)p->arch.kstack, 0, KSTACK_SIZE);

		/* User stack should be cloned before this, so we can use parent's
		 * ustack directly
		 */
		p->arch.ustack = parent->arch.ustack;
	} else {
		p->arch.ustack = 0;
	}
	p->arch.entry = 0;
	p->arch.size = 0;

	/* Initialize the file descriptor table */
	p->fds = fd_table_create(parent ? parent->fds : NULL);

	/* Initialize the death notifier */
	// TODO: Remember to wake up the waiters when we are going to die
	init_notifier(&p->death_notifier);

	p->status = 0;

	// TODO: Use a lock to protect this operation
	
	/* Insert this process into process tree */
	avl_tree_insert_node(&_proc_tree, &p->tree_link, p->id, p);
}

/**
 * Lookup a process of the specified pid
 */
struct process *process_lookup(pid_t pid)
{
	struct process *proc;

	// TODO: Use a lock to protect this operation
	proc = avl_tree_lookup(&_proc_tree, pid);

	return proc;
}

/**
 * Attach a thread to a process
 */
void process_attach(struct process *p, struct thread *t)
{
	t->owner = p;
	ASSERT(p->state != PROCESS_DEAD);
}

/**
 * Detach a thread from its owner
 */
void process_detach(struct thread *t)
{
	struct process *p;

	p = t->owner;
	t->owner = NULL;
}

/**
 * Destruct a process
 */
static void process_dtor(void *obj)
{
	struct process *p;

	p = (struct process *)obj;

	// TODO: Use a lock to protect this operation

	/* Remove this process from the process tree */
	avl_tree_remove_node(&_proc_tree, &p->tree_link);
	
	kfree((void *)p->arch.kstack - KSTACK_SIZE);

	// FixMe: cleanup the page directory owned by this process
	fd_table_destroy(p->fds);
}

/**
 * Switch to next process' context
 */
void process_switch(struct process *curr, struct process *prev)
{
	uint32_t esp, ebp, eip;

	/* Get the stack pointer and base pointer first */
	asm volatile("mov %%esp, %0" : "=r"(esp));
	asm volatile("mov %%ebp, %0" : "=r"(ebp));

	/* Read the instruction pointer. We do some cunning logic here:
	 * One of the two things could have happened when this function exits
	 *   (a) We called the function and it returned the EIP as requested.
	 *   (b) We have just switched processes, and because the saved EIP is
	 *       essentially the instruction after read_eip(), it will seem as
	 *       if read_eip has just returned.
	 * In the second case we need to return immediately. To detect it we put
	 * a magic number in EAX further down at the end of this function. As C
	 * returns value in EAX, it will look like the return value is this
	 * magic number. (0x87654321).
	 */
	eip = read_eip();
	
	/* If we have just switched processes, do nothing */
	if (eip == 0x87654321) {
		return;
	}

	/* Save the process context for previous process */
	if (prev) {
		prev->arch.eip = eip;
		prev->arch.esp = esp;
		prev->arch.ebp = ebp;
	}

	/* Switch to next process context */
	eip = curr->arch.eip;
	esp = curr->arch.esp;
	ebp = curr->arch.ebp;

	/* Make sure the memory manager knows we've changed the page directory */
	_current_mmu_ctx = curr->mmu_ctx;

	/* Switch the kernel stack in TSS to the process's kernel stack */
	set_kernel_stack(curr->arch.kstack);

	/* Here we:
	 * [1] Disable interrupts so we don't get bothered.
	 * [2] Temporarily puts the new EIP location in EBX.
	 * [3] Loads the stack and base pointers from the new process struct.
	 * [4] Changes page directory to the physical address of the new directory.
	 * [5] Puts a magic number (0x87654321) in EAX so that above we can recognize
	 *     that we've just switched process.
	 * [6] Enable interrupts. The STI instruction has a delay - it doesn't take
	 *     effect until after the next instruction
	 * [7] Jumps to the location in EBX (remember we put the new EIP in there)
	 * Note that you can't change the sequence we set each register here. You
	 * can check the actual asm code generated by your compiler for the reason.
	 */
	asm volatile ("mov %0, %%ebx\n"
		      "mov %1, %%esp\n"
		      "mov %2, %%ebp\n"
		      "mov %3, %%cr3\n"
		      "mov $0x87654321, %%eax\n"	/* read_eip() will return 0x87654321 */
		      "jmp *%%ebx\n"
		      :: "r"(eip), "r"(esp), "r"(ebp), "r"(_current_mmu_ctx->pdbr)
		      : "%ebx", "%esp", "%eax");
}

static void process_wait_notifier(void *data)
{
	struct process *p;

	p = (struct process *)data;
	p->state = PROCESS_RUNNING;
	sched_insert_proc(p);
}

int process_wait(struct process *p)
{
	int rc = -1;
	boolean_t state;
	
	ASSERT(p != NULL);
	if (p->state != PROCESS_DEAD) {
		notifier_register(&p->death_notifier, process_wait_notifier, CURR_PROC);

		/* Reschedule to next process */
		state = irq_disable();

		/* Set the process to wait state */
		CURR_PROC->state = PROCESS_WAIT;
		
		sched_reschedule(state);
		rc = 0;
	} else {
		rc = 0;
	}

	return rc;
}

void process_exit(int status)
{
	boolean_t state;

	CURR_PROC->status = status;

	DEBUG(DL_DBG, ("process_exit: id(%d), status(%d).\n", CURR_PROC->id, status));

	notifier_run(&CURR_PROC->death_notifier);

	state = irq_disable();

	/* Set the process to dead state */
	CURR_PROC->state = PROCESS_DEAD;

	sched_reschedule(state);
}

/* Relocate the stack, this must be done when you copy the stack to another place */
static void relocate_stack(uint32_t new_stack, uint32_t old_stack, uint32_t size)
{
	uint32_t i, tmp, off;

	if (new_stack > old_stack) {
		off = new_stack - old_stack;
	} else {
		off = old_stack - new_stack;
	}

	for (i = new_stack; i > (new_stack - size); i -= 4) {
		tmp = *((uint32_t *)i);
		if ((tmp < old_stack) && (tmp > (old_stack - size))) {
			uint32_t *tmp2;

			if (new_stack > old_stack) {
				tmp += off;
			} else {
				tmp -= off;
			}
			
			tmp2 = (uint32_t *)i;
			*tmp2 = tmp;
		}
	}
}

int fork()
{
	pid_t pid = 0;
	uint32_t magic = 'corP';
	struct process *parent;
	struct process *new_proc;
	struct mmu_ctx *ctx;
	uint32_t eip, esp, ebp;
	boolean_t state;
	
	state = irq_disable();

	/* Take a pointer to this process' process struct for later reference */
	parent = (struct process *)CURR_PROC;
	
	/* Clone the parent(current)'s page directory */
	ctx = mmu_create_ctx();
	mmu_copy_ctx(ctx, _current_mmu_ctx);

	/* Create a new process */
	new_proc = (struct process *)kmalloc(sizeof(struct process), 0);
	process_ctor(new_proc, parent, ctx);

	/* Enqueue the forked new process */
	sched_insert_proc(new_proc);
	DEBUG(DL_DBG, ("fork: parent(%p), child(%p).\n", parent, new_proc));
	
	eip = read_eip();

	/* We could be the parent or the child here */
	if (CURR_PROC == parent) {
		uint32_t offset;

		/* We are the parent, so setup the esp/ebp/eip for our child */
		asm volatile("mov %%esp, %0" : "=r"(esp));
		asm volatile("mov %%ebp, %0" : "=r"(ebp));

		/* Update the per process system call registers to the new process' one */
		if (parent->arch.kstack > new_proc->arch.kstack) {
			offset = parent->arch.kstack - new_proc->arch.kstack;
			new_proc->arch.esp = esp - offset;
			new_proc->arch.ebp = ebp - offset;
		} else {
			/* Can parent and child use the same stack? */
			ASSERT(parent->arch.kstack != new_proc->arch.kstack);
			offset = new_proc->arch.kstack - parent->arch.kstack;
			new_proc->arch.esp = esp + offset;
			new_proc->arch.ebp = ebp + offset;
		}
		
		new_proc->arch.eip = eip;

		/* Copy the kernel stack from parent process to the new process */
		memcpy((void *)new_proc->arch.kstack - KSTACK_SIZE,
		       (void *)parent->arch.kstack - KSTACK_SIZE,
		       KSTACK_SIZE);

		/* Relocate the kernel stack */
		relocate_stack(new_proc->arch.kstack, parent->arch.kstack, KSTACK_SIZE);

		DEBUG(DL_DBG, ("fork: kstack(0x%x -> 0x%x).\n",
			       parent->arch.kstack, new_proc->arch.kstack));

		offset = parent->arch.kstack - (uint32_t)parent->arch.syscall_regs;
		new_proc->arch.syscall_regs =
			(struct registers *)(new_proc->arch.kstack - offset);

		pid = new_proc->id;

		DEBUG(DL_DBG, ("fork: new process forked, pid %d\n"
			       "- esp(0x%x), ebp(0x%x), eip(0x%x), ustack(0x%x), mmu(0x%x)\n",
			       new_proc->id, new_proc->arch.esp, new_proc->arch.ebp,
			       new_proc->arch.eip, new_proc->arch.ustack,
			       new_proc->mmu_ctx));
		
		irq_restore(state);
	} else {
		/* We are the child, at this point we can't access the data even on
		 * the stack. We're just rescheduled.
		 */
		ASSERT(magic == 'corP');	// Make sure we have a correct stack
		DEBUG(DL_DBG, ("fork: child process.\n"));
	}
	
	return pid;
}

int exec(char *path, int argc, char **argv)
{
	int rc = -1;
	struct vfs_node *n;
	elf_ehdr_t *ehdr;
	uint32_t virt, entry;
	struct page *page;
	boolean_t is_elf;

	/* Lookup the file from the file system */
	n = vfs_lookup(path, VFS_FILE);
	if (!n) {
		DEBUG(DL_DBG, ("exec: file(%s) not found.\n", path));
		return -1;
	}

	/* Map some pages to the address from mmu context of this process, the mapped
	 * memory page is used for storing the ELF file content
	 */
	for (virt = 0x30000000;	virt < (0x30000000 + n->length); virt += PAGE_SIZE) {
		page = mmu_get_page(_current_mmu_ctx, virt, TRUE, 0);
		if (!page) {
			DEBUG(DL_ERR, ("exec: mmu_get_page failed, virt(0x%x).\n", virt));
			rc = -1;
			goto out;
		}
		
		page_alloc(page, FALSE, TRUE);
	}

	ehdr = (elf_ehdr_t *)0x30000000;

	/* Read in the executive content */
	rc = vfs_read(n, 0, n->length, (uint8_t *)ehdr);
	DEBUG(DL_DBG, ("exec: vfs_read done.\n"));
	if (rc == -1 || rc < sizeof(elf_ehdr_t)) {
		DEBUG(DL_INF, ("exec: read file(%s) failed.\n", path));
		rc = -1;
		goto out;
	}

	/* Check whether it is valid ELF */
	is_elf = elf_ehdr_check(ehdr);
	DEBUG(DL_DBG, ("exec: elf_ehdr_check done.\n"));
	if (!is_elf) {
		DEBUG(DL_INF, ("exec: invalid ELF, file(%s).\n", path));
		rc = -1;
		goto out;
	}

	/* Load the loadable segments from the executive to the address which was
	 * specified in the ELF and for Matrix, default is 0x20000000.
	 */
	rc = elf_load_sections(&(CURR_PROC->arch), ehdr);
	DEBUG(DL_DBG, ("exec: elf_load_sections done.\n"));
	if (rc != 0) {
		DEBUG(DL_WRN, ("exec: load sections failed, file(%s).\n", path));
		goto out;
	}

	/* Save the entry point to the code segment */
	entry = ehdr->e_entry;
	DEBUG(DL_DBG, ("exec: entry(0x%08x)\n", entry));

	/* Free the memory we used for the ELF headers and files */
	for (virt = 0x30000000; virt < (0x30000000 + n->length); virt += PAGE_SIZE) {
		page = mmu_get_page(_current_mmu_ctx, virt, FALSE, 0);
		ASSERT(page != NULL);
		page_free(page);
	}

	/* Map some pages for the user mode stack from current process' mmu context */
	for (virt = USTACK_BOTTOM; virt <= (USTACK_BOTTOM + USTACK_SIZE); virt += PAGE_SIZE) {
		page = mmu_get_page(_current_mmu_ctx, virt, TRUE, 0);
		if (!page) {
			DEBUG(DL_ERR, ("exec: mmu_get_page failed, virt(0x%x)\n", virt));
			rc = -1;
			goto out;
		}
		page_alloc(page, FALSE, TRUE);
	}

	/* Update the user stack */
	CURR_PROC->arch.ustack = (USTACK_BOTTOM + USTACK_SIZE);

	/* Jump to user mode */
	switch_to_user_mode(entry, USTACK_BOTTOM + USTACK_SIZE);

out:
	return -1;
}

int system(char *path, int argc, char **argv)
{
	int c;
	boolean_t state;

	c = fork();
	if (c == 0) {
		DEBUG(DL_DBG, ("system: child, c(%d).\n", c));
		exec(path, argc, argv);
		return -1;
	} else {
		state = irq_disable();
		ASSERT(state == TRUE);
		DEBUG(DL_DBG, ("system: parent, c(%d).\n", c));
		sched_reschedule(state);
		return -1;
	}
}

int getpid()
{
	return CURR_PROC->id;
}

/**
 * Switch to the user mode
 * @param location	- User address to jump to
 * @param ustack	- User stack
 */
void switch_to_user_mode(uint32_t location, uint32_t ustack)
{
	ASSERT(CURR_CPU == &_boot_cpu);
	
	/* Setup our kernel stack, note that the stack was grow from high address
	 * to low address
	 */
	set_kernel_stack(CURR_PROC->arch.kstack);
	
	/* Setup a stack structure for switching to user mode.
	 * The code firstly disables interrupts, as we're working on a critical
	 * section of code. It then sets the ds, es, fs and gs segment selectors
	 * to our user mode data selector - 0x23. Note that sti will not work when
	 * we enter user mode as it is a privileged instruction, we will set the
	 * interrupt flag to enable interrupt.
	 */
	asm volatile("cli\n"
		     "mov %1, %%esp\n"
		     "mov $0x23, %%ax\n"	/* Segment selector */
		     "mov %%ax, %%ds\n"
		     "mov %%ax, %%es\n"
		     "mov %%ax, %%fs\n"
		     "mov %%ax, %%gs\n"
		     "mov %%esp, %%eax\n"	/* Move stack to EAX */
		     "pushl $0x23\n"		/* Segment selector again */
		     "pushl %%eax\n"
		     "pushf\n"			/* Push flags */
		     "pop %%eax\n"		/* Enable the interrupt flag */
		     "orl $0x200, %%eax\n"
		     "push %%eax\n"
		     "pushl $0x1B\n"
		     "pushl %0\n"		/* Push the entry point */
		     "iret\n"
		     :: "m"(location), "r"(ustack) : "%ax", "%esp", "%eax");
}

/**
 * Start our kernel at top half
 */
void init_process()
{
	/* Relocate the stack so we know where it is, the stack size is 8KB. Note
	 * that this was done in the context of kernel mmu.
	 */
	move_stack(0xE0000000, KSTACK_SIZE);

	/* Malloc the initial process and initialize it, the initial process will
	 * use kernel mmu context as its mmu context.
	 */
	_kernel_proc = (struct process *)kmalloc(sizeof(struct process), 0);

	ASSERT(_current_mmu_ctx != NULL);
	process_ctor(_kernel_proc, NULL, _current_mmu_ctx);

	_kernel_proc->arch.kstack = 0xE0000000;
}
