/*
 * process.c
 */

#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/matrix.h"
#include "matrix/const.h"
#include "hal.h"
#include "mm/page.h"
#include "mm/mmu.h"
#include "mm/malloc.h"
#include "proc/process.h"
#include "matrix/debug.h"
#include "proc/sched.h"
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

extern uint32_t _initial_esp;
extern struct process *_next_proc;

extern uint32_t read_eip();
extern void sched_init();

static pid_t id_alloc()
{
	return _next_pid++;
}

void move_stack(void *new_stack, uint32_t size)
{
	uint32_t i, pd_addr;
	uint32_t old_esp, old_ebp;
	uint32_t new_esp, new_ebp;
	uint32_t offset;

	/* Allocate some space for the new stack */
	for (i = (uint32_t)new_stack;
	     i >= ((uint32_t)new_stack - size);
	     i -= 0x1000) {

		/* General purpose stack is in user-mode */
		struct page *page;
		
		page = mmu_get_page(_current_mmu_ctx, i, TRUE, 0);
		/* Associate the pte with a physical page */
		page_alloc(page, FALSE, TRUE);
	}

	/* Flush the TLB by reading and writing the page directory address again */
	asm volatile("mov %%cr3, %0" : "=r" (pd_addr));
	asm volatile("mov %0, %%cr3" :: "r" (pd_addr));

	/* Old ESP and EBP, read from registers */
	asm volatile("mov %%esp, %0" : "=r" (old_esp));
	asm volatile("mov %%ebp, %0" : "=r" (old_ebp));

	offset = (uint32_t)new_stack - _initial_esp;
	
	/* Initialize the new ESP and EBP */
	new_esp = old_esp + offset;
	new_ebp = old_ebp + offset;

	DEBUG(DL_DBG, ("move_stack: move stack to new address.\n"
		       "* old_esp(0x%x), old_ebp(0x%x)\n"
		       "* new_esp(0x%x), new_ebp(0x%x)\n"
		       "* _initial_esp(0x%x)\n",
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
	asm volatile("mov %0, %%esp" :: "r" (new_esp));
	asm volatile("mov %0, %%ebp" :: "r" (new_ebp));
}

/**
 * Construct a process
 */
static void process_ctor(void *obj, struct process *parent, struct mmu_ctx *ctx)
{
	struct process *p;

	p = (struct process *)obj;
	p->next = NULL;
	p->mmu_ctx = ctx;
	p->id = id_alloc();		// Allocate an ID for the process
	p->uid = 500;			// FixMe: set it to the currently logged user
	p->gid = 500;			// FixMe: set it to the current user's group
	p->priority = USER_Q;		// Default priority
	p->max_priority = PROCESS_Q;	// Max priority for the process
	p->quantum = P_QUANTUM;
	p->ticks_left = 0;		// Initial ticks is 0
	p->priv.flags = PREEMPTIBLE;
	p->usr_time = 0;
	p->sys_time = 0;
	p->name[0] = '\0';

	/* Initialize the architecture specific fields */
	p->arch.esp = 0;
	p->arch.ebp = 0;
	p->arch.eip = 0;
	p->arch.kstack = kmalloc(KSTACK_SIZE, MM_ALGN);
	// TODO: Get the right method to do this
	p->arch.ustack = 0;
	p->arch.size = 0;
	p->arch.entry = 0;

	/* Initialize the file descriptor table */
	p->fds = fd_table_create(parent ? parent->fds : NULL);
}

/**
 * Destruct a process
 */
static void process_dtor(void *obj)
{
	struct process *p;

	p = (struct process *)obj;
	
	kfree((void *)p->arch.kstack);
	// TODO: cleanup the page directory owned by this process
	fd_table_destroy(p->fds);
}

void switch_context()
{
	uint32_t esp, ebp, eip;

	asm volatile("mov %%esp, %0" : "=r" (esp));
	asm volatile("mov %%ebp, %0" : "=r" (ebp));

	/* Read the instruction pointer. We do some cunning logic here:
	 * One of the two things could have happened when this function exits
	 *   (a) We called the function and it returned the EIP as requested.
	 *   (b) We have just switched processes, and because the saved EIP is
	 *       essentially the instruction after read_eip(), it will seem as
	 *       if read_eip has just returned.
	 * In the second case we need to return immediately. To detect it we put
	 * a magic number in EAX further down at the end of this function. As C
	 * returns value in EAX, it will look like the return value is this
	 * magic number. (0x12345678).
	 */
	eip = read_eip();
	
	/* If we have just switched processes, do nothing */
	if (eip == 0x12345678) {
		return;
	}

	/* Save the current process context */
	CURR_PROC->arch.eip = eip;
	CURR_PROC->arch.esp = esp;
	CURR_PROC->arch.ebp = ebp;

	CURR_PROC = _next_proc;
	eip = CURR_PROC->arch.eip;
	esp = CURR_PROC->arch.esp;
	ebp = CURR_PROC->arch.ebp;

	/* Make sure the memory manager knows we've changed the page directory */
	_current_mmu_ctx = CURR_PROC->mmu_ctx;

	/* Switch the kernel stack in TSS to the process's kernel stack */
	set_kernel_stack(CURR_PROC->arch.kstack + KSTACK_SIZE);

	/* Here we:
	 * [1] Disable interrupts so we don't get bothered.
	 * [2] Temporarily puts the new EIP location in ECX.
	 * [3] Loads the stack and base pointers from the new process struct.
	 * [4] Changes page directory to the physical address of the new directory.
	 * [5] Puts a magic number (0x12345678) in EAX so that above we can recognize
	 *     that we've just switched process.
	 * [6] Enable interrupts. The STI instruction has a delay - it doesn't take
	 *     effect until after the next instruction
	 * [7] Jumps to the location in ECX (remember we put the new EIP in there)
	 * Note that you can't change the sequence we set each register here. You
	 * can check the actual asm code generated by your compiler for the reason.
	 */
	asm volatile ("cli");
	asm volatile ("mov %0, %%ecx" :: "r"(eip));
	asm volatile ("mov %0, %%esp" :: "r"(esp));
	asm volatile ("mov %0, %%ebp" :: "r"(ebp));
	asm volatile ("mov %0, %%cr3" :: "r"(_current_mmu_ctx->pdbr));
	asm volatile ("mov $0x12345678, %eax");
	asm volatile ("sti");
	asm volatile ("jmp *%ecx");
}

int fork()
{
	pid_t pid = 0;
	struct process *parent;
	struct process *new_process;
	struct mmu_ctx *ctx;
	uint32_t phys_addr;
	uint32_t eip;
	uint32_t esp;
	uint32_t ebp;
	
	irq_disable();

	/* Take a pointer to this process' process struct for later reference */
	parent = (struct process *)CURR_PROC;
	
	/* Clone the parent(current)'s page directory */
	ctx = mmu_create_ctx();
	mmu_copy_ctx(ctx, _current_mmu_ctx);

	/* Create a new process */
	new_process = (struct process *)kmalloc(sizeof(struct process), 0);
	process_ctor(new_process, parent, ctx);

	/* Enqueue the forked new process */
	sched_enqueue(new_process);
	
	eip = read_eip();

	/* We could be the parent or the child here */
	if (CURR_PROC == parent) {

		/* We are the parent, so setup the esp/ebp/eip for our child */
		asm volatile("mov %%esp, %0" : "=r" (esp));
		asm volatile("mov %%ebp, %0" : "=r" (ebp));

		new_process->arch.esp = esp;
		new_process->arch.ebp = ebp;
		new_process->arch.eip = eip;

		pid = new_process->id;

		DEBUG(DL_DBG, ("fork: new process forked, pid %d\n"
			       "- esp(0x%x), ebp(0x%x), eip(0x%x), page_dir(0x%x)\n",
			       new_process->id, new_process->arch.esp, new_process->arch.ebp,
			       new_process->arch.eip, new_process->mmu_ctx));
		
		irq_enable();
	} else {
		/* We are the child */
		DEBUG(DL_DBG, ("fork: now in child process.\n"));
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

	/* Lookup the file from the file system */
	n = vfs_lookup(path, 0);
	if (!n) {
		return -1;
	}

	/* Map some pages to the address for this process */
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
	if (rc == -1 || rc < sizeof(elf_ehdr_t)) {
		rc = -1;
		goto out;
	}

	/* Check whether it is valid ELF */
	if (!elf_ehdr_check(ehdr)) {
		rc = -1;
		goto out;
	}

	/* Load the loadable segments from the executive */
	rc = elf_load_sections(&(CURR_PROC->arch), ehdr);
	if (rc != 0) {
		goto out;
	}

	/* Save the entry point to the code segment */
	entry = ehdr->e_entry;

	/* Free the memory we used for the ELF headers and files */
	for (virt = 0x30000000; virt < (0x30000000 + n->length); virt += PAGE_SIZE) {
		page = mmu_get_page(_current_mmu_ctx, virt, FALSE, 0);
		ASSERT(page != NULL);
		page_free(page);
	}

	/* Map some pages to the user stack */
	for (virt = USTACK_BOTTOM; virt < (USTACK_BOTTOM + USTACK_SIZE); virt += PAGE_SIZE) {
		page = mmu_get_page(_current_mmu_ctx, virt, FALSE, 0);
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

	c = fork();
	if (c == 0) {
		exec(path, argc, argv);
		return -1;
	} else {
		switch_context();
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
	/* Setup our kernel stack, note that the stack was grow from high address
	 * to low address
	 */
	set_kernel_stack(CURR_PROC->arch.kstack + KSTACK_SIZE);
	
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
	struct process *p;

	/* Initialize the scheduler */
	init_sched();
	
	/* Relocate the stack so we know where it is, the stack size is 8KB */
	move_stack((void *)0xE0000000, 0x2000);

	/* Malloc the initial process and initialize it */
	p = (struct process *)kmalloc(sizeof(struct process), 0);
	process_ctor(p, NULL, _current_mmu_ctx);
	
	/* Enqueue the new process */
	sched_enqueue(p);

	/* Update the current process pointer */
	CURR_PROC = p;
}
