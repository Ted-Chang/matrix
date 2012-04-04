#ifndef __TASK_H__
#define __TASK_H__

#include <time.h>
#include "matrix/const.h"
#include "priv.h"


/* Our kernel stack size is 4096 bytes */
#define KERNEL_STACK_SIZE 4096

/* Definition of the architecture specific task structure */
struct arch_task {
	uint32_t esp;		// Stack pointer
	uint32_t ebp;		// Base pointer
	uint32_t eip;		// Instruction pointer
	uint32_t kernel_stack;	// Kernel stack location
};
typedef struct arch_task arch_task_t;

/* Definition of the task structure */
struct task {
	struct task *next;	// Next task
	struct priv priv;	// System privileges structure
	struct pd *page_dir;	// Page directory
	int id;			// Process ID
	struct arch_task arch;	// Architecture task implementation
	clock_t usr_time;	// User time in ticks
	clock_t sys_time;	// System time in ticks
	int8_t priority;	// Current scheduling priority
	int8_t max_priority;	// Max priority of the task
	int8_t ticks_left;	// Number of scheduling ticks left
	int8_t quantum;		// Quantum in ticks
	char name[T_NAME_LEN];	// Name of the task, include `\0'
};

/* Scheduling priority for our tasks. Value must start at zero (highest
 * priority) and increment.
 */
#define NR_SCHED_QUEUES		16
#define TASK_Q			0
#define MAX_USER_Q		0
#define USER_Q			7
#define MIN_USER_Q		14
#define IDLE_Q			15

/* Pointer to the current task in the system */
extern volatile struct task *_current_task;

void init_multitask();

void switch_context();

int fork();

int getpid();

void switch_to_user_mode();

#endif	/* __TASK_H__ */
