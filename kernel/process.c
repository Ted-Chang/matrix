/*
 * process.c
 */

#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/matrix.h"
#include "matrix/global.h"
#include "matrix/const.h"
#include "hal.h"
#include "mm/page.h"
#include "mm/mmu.h"
#include "mm/kmem.h"
#include "proc/process.h"
#include "matrix/debug.h"
#include "proc/sched.h"

process_id_t _next_pid = 1;

extern uint32_t _initial_esp;

extern uint32_t read_eip();

extern void sched_init();

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
		struct page *page = mmu_get_page(_current_mmu_ctx, i, TRUE, 0);
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
	struct process *p = (struct process *)obj;
	size_t nodes_len, i;
	
	p->next = NULL;
	p->mmu_ctx = ctx;
	p->id = _next_pid++;
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
	p->arch.kstack = kmem_alloc_a(KSTACK_SIZE);

	/* Initialize the file descriptor table */
	p->fds = (fd_table_t *)kmem_alloc(sizeof(fd_table_t));
	p->fds->ref_count = 1;
	if (!parent) {
		p->fds->len = 3;
	} else {
		p->fds->len = parent->fds->len;
	}
	nodes_len = p->fds->len * sizeof(struct vfs_node *);
	p->fds->nodes = (struct vfs_node **)kmem_alloc(nodes_len);
	if (parent) {			// Clone parent's file descriptors
		for (i = 0; i < p->fds->len; i++) {
			p->fds->nodes[i] = vfs_clone(parent->fds->nodes[i]);
		}
	}
	memset(p->fds->nodes, 0, nodes_len);
}

/**
 * Destruct a process
 */
static void process_dtor(void *obj)
{
	struct process *p = (struct process *)obj;
	kmem_free((void *)p->arch.kstack);
	// TODO: cleanup the page directory owned by this process
	// TODO: close the file descriptors opened by this process
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
	process_id_t pid = 0;
	struct process *parent;
	struct process *new_process;
	struct mmu_ctx *ctx;
	uint32_t phys_addr;
	uint32_t eip;
	uint32_t esp;
	uint32_t ebp;
	
	disable_interrupt();

	/* Take a pointer to this process' process struct for later reference */
	parent = (struct process *)CURR_PROC;
	
	/* Clone the parent(current)'s page directory */
	ctx = mmu_create_ctx();
	mmu_copy_ctx(ctx, _current_mmu_ctx);

	/* Create a new process */
	new_process = (struct process *)kmem_alloc(sizeof(struct process));
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
		
		enable_interrupt();
	} else {
		/* We are the child */
		DEBUG(DL_DBG, ("fork: now in child process.\n"));
	}
	
	return pid;
}

int getpid()
{
	return CURR_PROC->id;
}

void switch_to_user_mode()
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
	asm volatile("\
		     cli; \
		     mov $0x23, %ax; \
		     mov %ax, %ds; \
		     mov %ax, %es; \
		     mov %ax, %fs; \
		     mov %ax, %gs; \
		     \
		     mov %esp, %eax; \
		     pushl $0x23; \
		     pushl %esp; \
		     pushf; \
		     \
		     pop %eax; \
		     orl $0x200, %eax; \
		     push %eax; \
		     \
		     pushl $0x1B; \
		     push $1f; \
		     iret; \
		     1: ");
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
	p = (struct process *)kmem_alloc(sizeof(struct process));
	process_ctor(p, NULL, _current_mmu_ctx);
	
	/* Enqueue the new process */
	sched_enqueue(p);

	/* Update the current process pointer */
	CURR_PROC = p;
}
