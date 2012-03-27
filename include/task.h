#ifndef __TASK_H__
#define __TASK_H__

#include <time.h>
#include "priv.h"

/* Our kernel stack size is 4096 bytes */
#define KERNEL_STACK_SIZE 4096

/* Definition of the task structure */
struct task {
	struct task *next;	// Next task
	struct priv priv;	// System privileges structure
	struct pd *page_dir;	// Page directory
	int id;			// Process ID
	uint32_t esp;		// Stack pointer
	uint32_t ebp;		// Base pointer
	uint32_t eip;		// Instruction pointer
	uint32_t kernel_stack;	// Kernel stack location
	clock_t usr_time;	// User time in ticks
	clock_t sys_time;	// System time in ticks
	uint8_t priority;	// Current scheduling priority
	int8_t ticks_left;	// Number of scheduling ticks left
	int8_t quantum;		// Quantum in ticks
};

/* Pointer to the current task in the system */
extern volatile struct task *_current_task;

void init_multitask();

void switch_task();

int fork();

int getpid();

void switch_to_user_mode();

#endif	/* __TASK_H__ */
