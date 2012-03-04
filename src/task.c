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
		
		DEBUG(DL_DBG, ("move_stack: addr(0x%x), frame number(0x%x)\n",
			       i, page->frame));
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

	DEBUG(DL_DBG, ("move_stack: old_esp(0x%x), old_ebp(0x%x), offset(0x%x)\n"
		       "            initial_esp(0x%x) new_stack(0x%x)\n",
		       old_esp, old_ebp, offset, initial_esp, new_stack));

	/* Copy the stack */
	memcpy((void *)new_stack, (void *)old_esp, initial_esp - old_esp);

	/* Backtrace through the original stack, copying new values into
	 * the new stack
	 */
	for (i = (uint32_t)new_stack; i > (uint32_t)new_stack - size; i -= 4) {
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

	/* Make changes to the stack */
	asm volatile("mov %0, %%esp" :: "r" (new_esp));
	asm volatile("mov %0, %%ebp" :: "r" (new_ebp));
}

void init_multitask()
{
	/* No interrupts at this time */
	disable_interrupt();

	/* Relocate the stack so we know where it is */
	move_stack((void *)0xE0000000, 0x2000);

	/* Malloc the initial task and initialize it */
	current_task = ready_queue = (struct task *)kmalloc(sizeof(struct task));
	current_task->id = next_pid++;
	current_task->esp = current_task->ebp = 0;
	current_task->eip = 0;
	current_task->page_dir = current_dir;
	current_task->next = 0;
	
	enable_interrupt();
}

void switch_task()
{
	uint32_t esp, ebp, eip;
	
	/* If we haven't initialized multitask yet, do nothing */
	if (!current_task)
		return;

	asm volatile("mov %%esp, %0" : "=r" (esp));
	asm volatile("mov %%ebp, %0" : "=r" (ebp));
	
	eip = read_eip();

	/* If we have just switched tasks, do nothing */
	if (eip == 0x12345)
		return;

	/* Save the current process context */
	current_task->eip = eip;
	current_task->esp = esp;
	current_task->ebp = ebp;

	/* Get the next ready task to run */
	current_task = current_task->next;
	if (!current_task)
		current_task = ready_queue;

	eip = current_task->eip;
	esp = current_task->esp;
	ebp = current_task->ebp;

	current_dir = current_task->page_dir;

	asm volatile(
		     "cli; \
		     mov %0, %%ecx; \
		     mov %1, %%esp; \
		     mov %2, %%ebp; \
		     mov %3, %%cr3; \
		     mov $0x12345, %%eax; \
		     sti; \
		     jmp *%%ecx" \
		     :: "r"(eip), "r"(esp), "r"(ebp), "r"(current_dir->physical_addr));
}

int fork()
{
	struct task *parent;
	struct pd *dir;
	struct task *new_task;
	struct task *tmp_task;
	uint32_t eip;
	
	disable_interrupt();

	parent = (struct task *)current_task;
	dir = clone_pd(current_dir);
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

	/* Get the entry point of the new task */
	eip = read_eip();

	/* We could be the parent or the child here */
	if (current_task == parent) {
		uint32_t esp;
		uint32_t ebp;

		asm volatile("mov %%esp, %0" : "=r" (esp));
		asm volatile("mov %%ebp, %0" : "=r" (ebp));

		new_task->esp = esp;
		new_task->ebp = ebp;
		new_task->eip = eip;

		enable_interrupt();
		
		return new_task->id;
	}

	/* We are the child */
	return 0;
}

int getpid()
{
	return current_task->id;
}
