#ifndef __TASK_H__
#define __TASK_H__

/* Our kernel stack size is 4096 bytes */
#define KERNEL_STACK_SIZE 4096

/* Definition of the task structure */
struct task {
	int id;			// Process ID
	uint32_t esp;		// Stack pointer
	uint32_t ebp;		// Base pointer
	uint32_t eip;		// Instruction pointer
	struct pd *page_dir;	// Page directory
	uint32_t kernel_stack;	// Kernel stack location
	struct task *next;
};

void init_multitask();

void task_switch();

int fork();

int getpid();

#endif	/* __TASK_H__ */
