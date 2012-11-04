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
#include "object.h"

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

/* kernel process */
struct process *_kernel_proc = NULL;

extern uint32_t _initial_esp;

static pid_t id_alloc()
{
	return _next_pid++;
}

/* Move stack to a new position */
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

/**
 * Construct a process
 */
static void process_ctor(void *obj)
{
	struct process *p;

	p = (struct process *)obj;

	/* Initialize the signature */
	p->ref_count = 0;

	/* if (parent) { */
	/* 	/\* User stack should be cloned before this, so we can use parent's */
	/* 	 * ustack directly */
	/* 	 *\/ */
	/* 	p->arch.ustack = parent->arch.ustack; */
	/* } else { */
	/* 	p->arch.ustack = 0; */
	/* } */

	/* Initialize the death notifier */
	init_notifier(&p->death_notifier);

	p->status = 0;
}

/**
 * Destruct a process
 */
static void process_dtor(void *obj)
{
	DEBUG(DL_DBG, ("process_dtor: process (%p) destructed.\n", obj));
}

static int process_alloc(const char *name, struct process *parent, struct mmu_ctx *mmu,
			 int flags, int priority, struct fd_table *fds,
			 struct process **procp)
{
	int rc = -1;
	struct process *p;

	/* Create the new process */
	p = kmalloc(sizeof(struct process), 0);
	if (!p) {
		DEBUG(DL_DBG, ("process_create: kmalloc process failed.\n"));
		rc = -1;
		goto out;
	}
	memset(p, 0, sizeof(struct process));

	/* Process constructor */
	process_ctor(p);

	/* Allocate a process ID. If creating the kernel process, always give
	 * it an ID of 0.
	 */
	if (_kernel_proc) {
		p->id = id_alloc();		// Allocate an ID for the process
	} else {
		p->id = 0;
	}
	
	strncpy(p->name, name, P_NAME_LEN - 1);

	p->uid = 500;			// FixMe: set it to the currently logged user
	p->gid = 500;			// FixMe: set it to the current user's group
	p->mmu_ctx = mmu;		// MMU context
	p->priority = priority;
	p->flags = flags;
	p->status = 0;
	
	/* Initialize the file descriptor table */
	if (!fds) {
		p->fds = fd_table_create();
		if (!p->fds) {
			rc = -1;
			goto out;
		}
	} else {
		p->fds = fds;
	}

	/* Insert this process into process tree */
	avl_tree_insert_node(&_proc_tree, &p->tree_link, p->id, p);

	p->state = PROCESS_RUNNING;
	*procp = p;
	rc = 0;

	DEBUG(DL_DBG, ("process_alloc: created process (%s:%d:%p).\n",
		       p->name, p->id, p));
out:
	if (rc != 0) {
		if (p) {
			kfree(p);
		}
	}
	
	return rc;
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
	list_add_tail(&t->owner_link, &p->threads);
}

/**
 * Detach a thread from its owner
 */
void process_detach(struct thread *t)
{
	struct process *p;

	p = t->owner;
	list_del(&t->owner_link);

	/* Move the process to the dead state if no threads is alive */
	if (LIST_EMPTY(&p->threads)) {
		ASSERT(p->state != PROCESS_DEAD);
		p->state = PROCESS_DEAD;
	}
	
	t->owner = NULL;
}

void process_exit(int status)
{
	struct thread *t;
	struct list *l, *p;
	
	LIST_FOR_EACH_SAFE(l, p, &CURR_PROC->threads) {
		t = LIST_ENTRY(l, struct thread, owner_link);
		if (t != CURR_THREAD) {
			thread_kill(t);
		}
	}

	CURR_PROC->status = status;

	thread_exit();
}

static void process_entry_thread(void *ctx)
{
	;
}

int process_create(const char *args[], struct process *parent, int flags,
		   int priority, struct process **procp)
{
	int rc = -1;
	struct process *p = NULL;
	struct mmu_ctx *mmu = NULL;
	struct thread *t = NULL;

	if (priority >= 32) {
		DEBUG(DL_DBG, ("process_create: invalid parameter.\n"));
		rc = -1;
		goto out;
	}

	/* Create the address space for the process */
	mmu = mmu_create_ctx();
	if (!mmu) {
		DEBUG(DL_INF, ("process_create: mmu_create_ctx failed.\n"));
		rc = -1;
		goto out;
	}
	mmu_copy_ctx(mmu, &_kernel_mmu_ctx);

	/* Create the new process */
	rc = process_alloc(args[0], parent, mmu, flags, priority, NULL, &p);
	if (rc != 0) {
		DEBUG(DL_INF, ("process_create: process_alloc failed, err(%d).\n", rc));
		goto out;
	}

	/* Create the main thread and run it */
	rc = thread_create(p, 0, process_entry_thread, NULL, &t);
	if (rc != 0) {
		DEBUG(DL_INF, ("process_create: thread_create failed, err(%d).\n", rc));
		goto out;
	}

	thread_run(t);
	thread_release(t);

	if (procp) {
		*procp = p;
	}
	rc = 0;		// We'are OK

out:
	if (rc != 0) {
		if (p) {
			process_destroy(p);
		}
		if (mmu) {
			mmu_destroy_ctx(mmu);
		}
	}

	return rc;
}

int process_destroy(struct process *proc)
{
	ASSERT(LIST_EMPTY(&proc->threads));
	
	/* Remove this process from the process tree */
	avl_tree_remove_node(&_proc_tree, &proc->tree_link);

	notifier_clear(&proc->death_notifier);
	
	kfree(proc);

	return 0;
}

int process_wait(struct process *p, void *sync)
{
	int rc = -1;
	boolean_t state;

	if (p->state != PROCESS_DEAD) {
		/* Register us to the death notifier of the specified process */
		notifier_register(&p->death_notifier, object_wait_notifier, sync);
		rc = 0;
	} else {
		DEBUG(DL_INF, ("process_wait: process(%d) dead.", p->id));
		rc = -1;
	}

	return rc;
}

int process_clone()
{
	int rc = -1;
	struct process *p = NULL;
	struct thread *t = NULL;
	struct mmu_ctx *mmu;
	struct fd_table *fds = NULL;
	
	/* Clone the parent(current)'s page directory */
	mmu = mmu_create_ctx();
	if (!mmu) {
		DEBUG(DL_DBG, ("process_clone: mmu_create_ctx failed.\n"));
		goto out;
	}
	mmu_copy_ctx(mmu, CURR_PROC->mmu_ctx);

	/* Clone the parent(current)'s file descriptor table */
	fds = fd_table_clone(CURR_PROC->fds);
	if (!fds) {
		DEBUG(DL_DBG, ("process_clone: fd_table_clone failed.\n"));
		goto out;
	}

	/* Create the new process and a handle */
	rc = process_alloc(CURR_PROC->name, CURR_PROC, mmu, 0, 16, fds, &p);
	if (rc != 0) {
		DEBUG(DL_DBG, ("process_clone: process_alloc failed, err(%d).\n", rc));
		goto out;
	}

	/* Create the main thread and run it */
	rc = thread_create(p, 0, process_entry_thread, NULL, &t);
	if (rc != 0) {
		DEBUG(DL_DBG, ("process_clone: thread_create failed, err(%d).\n", rc));
		goto out;
	}

	thread_run(t);
	thread_release(t);

out:
	if (rc != 0) {
		if (p) {
			process_destroy(p);
		}
		if (fds) {
			fd_table_destroy(fds);
		}
		if (mmu) {
			/* TODO: Should unmap mmu context */
			mmu_destroy_ctx(mmu);
		}
	}
	
	return rc;
}

int process_replace(const char *path, const char *args[])
{
	return -1;
}

int fork()
{
	/* Take a pointer to this process' process struct for later reference */
	/* parent = (struct process *)CURR_PROC; */
	/* ASSERT(parent != NULL); */
	
	/* eip = read_eip(); */

	/* /\* We could be the parent or the child here *\/ */
	/* if (CURR_PROC == parent) { */
	/* 	uint32_t offset; */

	/* 	/\* We are the parent, so setup the esp/ebp/eip for our child *\/ */
	/* 	asm volatile("mov %%esp, %0" : "=r"(esp)); */
	/* 	asm volatile("mov %%ebp, %0" : "=r"(ebp)); */

	/* 	/\* Update the per process system call registers to the new process' one *\/ */
	/* 	if (parent->arch.kstack > new_proc->arch.kstack) { */
	/* 		offset = parent->arch.kstack - new_proc->arch.kstack; */
	/* 		new_proc->arch.esp = esp - offset; */
	/* 		new_proc->arch.ebp = ebp - offset; */
	/* 	} else { */
	/* 		/\* Can parent and child use the same stack? *\/ */
	/* 		ASSERT(parent->arch.kstack != new_proc->arch.kstack); */
	/* 		offset = new_proc->arch.kstack - parent->arch.kstack; */
	/* 		new_proc->arch.esp = esp + offset; */
	/* 		new_proc->arch.ebp = ebp + offset; */
	/* 	} */
		
	/* 	new_proc->arch.eip = eip; */

	/* 	/\* Copy the kernel stack from parent process to the new process *\/ */
	/* 	memcpy((void *)new_proc->arch.kstack - KSTACK_SIZE, */
	/* 	       (void *)parent->arch.kstack - KSTACK_SIZE, */
	/* 	       KSTACK_SIZE); */

	/* 	/\* Relocate the kernel stack *\/ */
	/* 	relocate_stack(new_proc->arch.kstack, parent->arch.kstack, KSTACK_SIZE); */

	/* 	DEBUG(DL_DBG, ("fork: p(%p:%p) kstk(%p:%p) mmu(%p:%p)\n", */
	/* 		       parent, new_proc, parent->arch.kstack, new_proc->arch.kstack, */
	/* 		       parent->mmu_ctx, new_proc->mmu_ctx)); */

	/* 	offset = parent->arch.kstack - (uint32_t)parent->arch.syscall_regs; */
	/* 	new_proc->arch.syscall_regs = */
	/* 		(struct registers *)(new_proc->arch.kstack - offset); */

	/* 	pid = new_proc->id; */

	/* 	DEBUG(DL_DBG, ("fork: new process, pid(%d)\n" */
	/* 		       "- esp(0x%x) ebp(0x%x) eip(0x%x) ustk(0x%x)\n", */
	/* 		       new_proc->id, new_proc->arch.esp, new_proc->arch.ebp, */
	/* 		       new_proc->arch.eip, new_proc->arch.ustack)); */

	/* 	/\* This must be done after we have initialized all field of the new process *\/ */
	/* 	//sched_insert_proc(new_proc); */
		
	/* 	irq_restore(state); */
	/* } else { */
	/* 	/\* We are the child, at this point we can't access the data even on */
	/* 	 * the stack. We're just rescheduled. */
	/* 	 *\/ */
	/* 	DEBUG(DL_DBG, ("fork: child process.\n")); */
	/* } */
	
	return 0;
}

int exec(const char *path, int argc, char **argv)
{
	int rc = -1;
	struct vfs_node *n;
	elf_ehdr_t *ehdr;
	uint32_t virt, entry, temp;
	struct page *page;
	boolean_t is_elf;

	/* Lookup the file from the file system */
	n = vfs_lookup(path, VFS_FILE);
	if (!n) {
		DEBUG(DL_DBG, ("exec: file(%s) not found.\n", path));
		return -1;
	}

	/* Temporarily use address of user stack to load the ELF file, It will be
	 * unmapped after we loaded the code section in.
	 */
	temp = USTACK_BOTTOM;
	
	/* Map some pages to the address from mmu context of this process, the mapped
	 * memory page is used for storing the ELF file content
	 */
	for (virt = temp; virt < (temp + n->length); virt += PAGE_SIZE) {
		page = mmu_get_page(CURR_PROC->mmu_ctx, virt, TRUE, 0);
		if (!page) {
			DEBUG(DL_ERR, ("exec: mmu_get_page failed, virt(0x%x).\n", virt));
			rc = -1;
			goto out;
		}
		
		page_alloc(page, FALSE, TRUE);
	}

	ehdr = (elf_ehdr_t *)temp;

	/* Read in the executive content */
	rc = vfs_read(n, 0, n->length, (uint8_t *)ehdr);
	if (rc == -1 || rc < sizeof(elf_ehdr_t)) {
		DEBUG(DL_INF, ("exec: read file(%s) failed.\n", path));
		rc = -1;
		goto out;
	}

	/* Check whether it is valid ELF */
	is_elf = elf_ehdr_check(ehdr);
	if (!is_elf) {
		DEBUG(DL_INF, ("exec: invalid ELF, file(%s).\n", path));
		rc = -1;
		goto out;
	}

	/* Load the loadable segments from the executive to the address which was
	 * specified in the ELF and for Matrix. default is 0x40000000 which was
	 * specified in the link script.
	 */
	rc = elf_load_sections(NULL/* TODO: FixMe */, ehdr);
	if (rc != 0) {
		DEBUG(DL_WRN, ("exec: load sections failed, file(%s).\n", path));
		goto out;
	}

	/* Save the entry point to the code segment */
	entry = ehdr->e_entry;
	DEBUG(DL_DBG, ("exec: entry(%p)\n", entry));
	ASSERT(entry != USTACK_BOTTOM);

	/* Free the memory we used for temporarily store the ELF files */
	for (virt = temp; virt < (temp + n->length); virt += PAGE_SIZE) {
		page = mmu_get_page(CURR_PROC->mmu_ctx, virt, FALSE, 0);
		ASSERT(page != NULL);
		page_free(page);
	}

	/* Map some pages for the user mode stack from current process' mmu context */
	for (virt = USTACK_BOTTOM; virt <= (USTACK_BOTTOM + USTACK_SIZE); virt += PAGE_SIZE) {
		page = mmu_get_page(CURR_PROC->mmu_ctx, virt, TRUE, 0);
		if (!page) {
			DEBUG(DL_ERR, ("exec: mmu_get_page failed, virt(0x%x)\n", virt));
			rc = -1;
			goto out;
		}
		page_alloc(page, FALSE, TRUE);
	}

	/* Update the user stack */
	CURR_THREAD->ustack = (void *)(USTACK_BOTTOM + USTACK_SIZE);

	/* Jump to user mode */
	switch_to_user_mode(entry, USTACK_BOTTOM + USTACK_SIZE);

out:
	return -1;
}

int process_getpid()
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
	set_kernel_stack(CURR_THREAD->kstack);
	
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
	int rc;
	
	/* Relocate the stack so we know where it is, the stack size is 8KB. Note
	 * that this was done in the context of kernel mmu.
	 */
	move_stack(0xE0000000, KSTACK_SIZE);

	/* Initialize the process avl tree */
	avl_tree_init(&_proc_tree);

	/* Create the kernel process */
	rc = process_alloc("kernel", NULL, NULL, 0, 16, NULL, &_kernel_proc);
	if (rc != 0) {
		PANIC("Could not initialize kernel process");
	}
}
