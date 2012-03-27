/*
 * task.c
 */

#include <types.h>
#include "matrix.h"
#include "hal.h"
#include "string.h"
#include "mmgr.h"
#include "task.h"
#include "debug.h"

volatile struct task *_current_task = NULL;

volatile struct task *_ready_queue = NULL;

uint32_t _next_pid = 1;

extern struct pd *_kernel_dir;
extern struct pd *_current_dir;

extern uint32_t _initial_esp;

extern uint32_t read_eip();

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
		struct pte *page = get_pte(i, TRUE, _current_dir);
		/* Associate the pte with a physical page */
		alloc_frame(page, FALSE, TRUE);
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

void init_multitask()
{
	/* No interrupts at this time */
	disable_interrupt();

	/* Relocate the stack so we know where it is, the stack size is 8KB */
	move_stack((void *)0xE0000000, 0x2000);

	/* Malloc the initial task and initialize it */
	_current_task = _ready_queue = (struct task *)kmalloc(sizeof(struct task));
	_current_task->id = _next_pid++;
	_current_task->esp = _current_task->ebp = 0;
	_current_task->eip = 0;
	_current_task->page_dir = _current_dir;
	_current_task->next = NULL;
	_current_task->kernel_stack = kmalloc_a(KERNEL_STACK_SIZE);
	_current_task->quantum = 10;			// Default quantum: 10 ticks
	_current_task->ticks_left = _current_task->quantum;
	_current_task->priv.flags = PREEMPTIBLE;	// The task is preemptible
	_current_task->usr_time = 0;
	_current_task->sys_time = 0;

	enable_interrupt();
}

void switch_task()
{
	uint32_t esp, ebp, eip;

	/* If we haven't initialized multitasking yet, do nothing */
	if (!_current_task)
		return;

	/* We will not switch task if the process didn't use up a full quantum. */
	if (!((_current_task->ticks_left <= 0) &&
	      (FLAG_ON(_current_task->priv.flags, PREEMPTIBLE))))
		return;

	/* Give the current task a new quantum as it will be switched */
	_current_task->ticks_left = _current_task->quantum;

	asm volatile("mov %%esp, %0" : "=r" (esp));
	asm volatile("mov %%ebp, %0" : "=r" (ebp));

	/* Read the instruction pointer. We do some cunning logic here:
	 * One of the two things could have happened when this function exits
	 *   (a) We called the function and it returned the EIP as requested.
	 *   (b) We have just switched tasks, and because the saved EIP is
	 *       essentially the instruction after read_eip(), it will seem as
	 *       if read_eip has just returned.
	 * In the second case we need to return immediately. To detect it we put
	 * a magic number in EAX further down at the end of this function. As C
	 * returns value in EAX, it will look like the return value is this
	 * magic number. (0x12345678).
	 */
	eip = read_eip();
	
	/* If we have just switched tasks, do nothing */
	if (eip == 0x12345678) {
		return;
	}

	/* Save the current process context */
	_current_task->eip = eip;
	_current_task->esp = esp;
	_current_task->ebp = ebp;

	/* Get the next ready task to run */
	_current_task = _current_task->next;

	/* If we get to the end of the task list start again at the beginning */
	if (!_current_task) _current_task = _ready_queue;

	eip = _current_task->eip;
	esp = _current_task->esp;
	ebp = _current_task->ebp;

	/* Make sure the memory manager knows we've changed the page directory */
	_current_dir = _current_task->page_dir;

	/* Switch the kernel stack in TSS to the task's kernel stack */
	set_kernel_stack(_current_task->kernel_stack + KERNEL_STACK_SIZE);

	/* Here we:
	 * [1] Disable interrupts so we don't get bothered.
	 * [2] Temporarily puts the new EIP location in ECX.
	 * [3] Loads the stack and base pointers from the new task struct.
	 * [4] Changes page directory to the physical address of the new directory.
	 * [5] Puts a magic number (0x12345678) in EAX so that above we can recognize
	 *     that we've just switched task.
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
	asm volatile ("mov %0, %%cr3" :: "r"(_current_dir->physical_addr));
	asm volatile ("mov $0x12345678, %eax");
	asm volatile ("sti");
	asm volatile ("jmp *%ecx");
}

int fork()
{
	int pid = 0;
	struct task *parent;
	struct pd *dir;
	struct task *new_task;
	struct task *tmp_task;
	uint32_t eip;
	uint32_t esp;
	uint32_t ebp;
	
	disable_interrupt();

	/* Take a pointer to this process' task struct for later reference */
	parent = (struct task *)_current_task;
	
	/* Clone the parent(current)'s page directory */
	dir = clone_pd(_current_dir);

	/* Create a new task */
	new_task = (struct task *)kmalloc(sizeof(struct task));

	new_task->id = _next_pid++;
	new_task->esp = new_task->ebp = 0;
	new_task->eip = 0;
	new_task->page_dir = dir;
	new_task->next = 0;
	new_task->kernel_stack = kmalloc_a(KERNEL_STACK_SIZE);
	new_task->quantum = 10;
	new_task->ticks_left = new_task->quantum;
	new_task->priv.flags = PREEMPTIBLE;
	new_task->usr_time = 0;
	new_task->sys_time = 0;

	/* Add the new task to the end of the ready queue */
	tmp_task = (struct task *)_ready_queue;
	while (tmp_task->next)
		tmp_task = tmp_task->next;
	tmp_task->next = new_task;

	eip = read_eip();

	/* We could be the parent or the child here */
	if (_current_task == parent) {

		/* We are the parent, so setup the esp/ebp/eip for our child */
		asm volatile("mov %%esp, %0" : "=r" (esp));
		asm volatile("mov %%ebp, %0" : "=r" (ebp));

		new_task->esp = esp;
		new_task->ebp = ebp;
		new_task->eip = eip;

		pid = new_task->id;

		DEBUG(DL_DBG, ("fork: new task forked, pid %d\n"
			       "- esp(0x%x), ebp(0x%x), eip(0x%x), page_dir(0x%x)\n",
			       new_task->id, new_task->esp, new_task->ebp, new_task->eip,
			       new_task->page_dir));
		
		enable_interrupt();
	} else {
		/* We are the child */
		DEBUG(DL_DBG, ("fork: now in child process.\n"));
	}
	
	return pid;
}

int getpid()
{
	return _current_task->id;
}

void switch_to_user_mode()
{
	/* Setup our kernel stack, note that the stack was grow from high address
	 * to low address
	 */
	set_kernel_stack(_current_task->kernel_stack + KERNEL_STACK_SIZE);
	
	/* Setup a stack structure for switching to user mode.
	 * The code firstly disables interrupts, as we're working on a critical
	 * section of code. It then sets the ds, es, fs and gs segment selectors
	 * to our user mode data selector - 0x23. 
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
		     pushl $0x1B; \
		     push $1f; \
		     iret; \
		     1: ");
}