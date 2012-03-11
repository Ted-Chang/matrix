/*
 * task.c
 */

#include "types.h"
#include "hal.h"
#include "mmgr.h"
#include "task.h"
#include "debug.h"

volatile struct task *current_task = 0;

volatile struct task *ready_queue = 0;

uint32_t next_pid = 1;

extern struct pd *kernel_dir;
extern struct pd *current_dir;

extern uint32_t initial_esp;

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
		struct pte *page = get_pte(i, TRUE, current_dir);
		/* Associate the pte with a physical page */
		alloc_frame(page, FALSE, TRUE);
	}

	/* Flush the TLB by reading and writing the page directory address again */
	asm volatile("mov %%cr3, %0" : "=r" (pd_addr));
	asm volatile("mov %0, %%cr3" :: "r" (pd_addr));

	/* Old ESP and EBP, read from registers */
	asm volatile("mov %%esp, %0" : "=r" (old_esp));
	asm volatile("mov %%ebp, %0" : "=r" (old_ebp));

	offset = (uint32_t)new_stack - initial_esp;
	
	/* Initialize the new ESP and EBP */
	new_esp = old_esp + offset;
	new_ebp = old_ebp + offset;

	DEBUG(DL_DBG, ("move_stack: old_esp(0x%x), old_ebp(0x%x),\n"
		       "\tnew_esp(0x%x), new_ebp(0x%x), initial_esp(0x%x)\n",
		       old_esp, old_ebp, new_esp, new_ebp, initial_esp));
	
	/* Copy the old stack to new stack. Although we have switched to cloned
	 * page directory, the kernel memory map was not changed. So we can just
	 * copy the data between the initial esp and the old esp to our new stack.
	 */
	memcpy((void *)new_esp, (void *)old_esp, initial_esp - old_esp);

	/* Backtrace through the original stack, copying new values into
	 * the new stack
	 */
	for (i = (uint32_t)new_stack; i > ((uint32_t)new_stack - size); i -= 4) {
		uint32_t tmp = *(uint32_t *)i;

		/* If the value of tmp is inside the range of the old stack, assume
		 * it is a base pointer and remap it. This will unfortunately remap
		 * ANY value in this range, whether they are base pointers or not.
		 */
		if ((old_esp < tmp) && (tmp < initial_esp)) {
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
	current_task = ready_queue = (struct task *)kmalloc(sizeof(struct task));
	current_task->id = next_pid++;
	current_task->esp = current_task->ebp = 0;
	current_task->eip = 0;
	current_task->page_dir = current_dir;
	current_task->next = NULL;

	enable_interrupt();
}

void switch_task()
{
	uint32_t esp, ebp, eip;
	
	/* If we haven't initialized multitasking yet, do nothing */
	if (!current_task)
		return;

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
	current_task->eip = eip;
	current_task->esp = esp;
	current_task->ebp = ebp;

	/* Get the next ready task to run */
	current_task = current_task->next;

	/* If we get to the end of the task list start again at the beginning */
	if (!current_task) current_task = ready_queue;

	eip = current_task->eip;
	esp = current_task->esp;
	ebp = current_task->ebp;

	/* Make sure the memory manager knows we've changed the page directory */
	current_dir = current_task->page_dir;

	//DEBUG(DL_DBG, ("switch_task: switching to task %d\n"
	//	       "\tesp(0x%x), ebp(0x%x), eip(0x%x), page_dir(0x%x).\n",
	//	       current_task->id, esp, ebp, eip, current_dir));

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
	asm volatile ("mov %0, %%cr3" :: "r"(current_dir->physical_addr));
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
	parent = (struct task *)current_task;
	
	/* Clone the parent(current)'s page directory */
	dir = clone_pd(current_dir);

	/* Create a new task */
	new_task = (struct task *)kmalloc(sizeof(struct task));

	new_task->id = next_pid++;
	new_task->esp = new_task->ebp = 0;
	new_task->eip = 0;
	new_task->page_dir = dir;
	new_task->next = 0;

	/* Add the new task to the end of the ready queue */
	tmp_task = (struct task *)ready_queue;
	while (tmp_task->next)
		tmp_task = tmp_task->next;
	tmp_task->next = new_task;

	eip = read_eip();

	/* We could be the parent or the child here */
	if (current_task == parent) {

		/* We are the parent, so setup the esp/ebp/eip for our child */
		asm volatile("mov %%esp, %0" : "=r" (esp));
		asm volatile("mov %%ebp, %0" : "=r" (ebp));

		new_task->esp = esp;
		new_task->ebp = ebp;
		new_task->eip = eip;

		pid = new_task->id;

		DEBUG(DL_DBG, ("fork: new_task - id %d\n"
			       "\tesp(0x%x), ebp(0x%x), eip(0x%x), page_dir(0x%x)\n",
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
	return current_task->id;
}

void switch_to_user_mode()
{
	/* Setup a stack structure for switching to user mode.
	 * The code firstly disables interrupts, as we're working on a critical
	 * section of code. It then sets the ds, es, fs and gs segment selectors
	 * to our user mode data selector - 0x23. 
	 */
	asm volatile("cli");
	asm volatile("mov $0x23, %ax");
	asm volatile("mov %ax, %ds");
	asm volatile("mov %ax, %es");
	asm volatile("mov %ax, %fs");
	asm volatile("mov %ax, %gs");
	
	asm volatile("mov %esp, %eax");	// Saves the stack point in EAX
	asm volatile("pushl $0x23");
	asm volatile("pushl %eax");
	asm volatile("pushf");		// Pushes current value of EFLAGS
	asm volatile("pushl $0x1B");	// Pushes the CS selector value
	asm volatile("push $1f");	// Pushes the the value of next label onto the stack
	asm volatile("iret");
	asm volatile("1:");
}
