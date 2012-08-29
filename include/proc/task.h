#ifndef __TASK_H__
#define __TASK_H__

#include <sys/time.h>
#include "matrix/const.h"
#include "priv.h"
#include "fs.h"


/* Our kernel stack size is 4096 bytes */
#define KSTACK_SIZE 4096

/* Definition of the architecture specific task structure */
struct arch_task {
	uint32_t esp;		// Stack pointer
	uint32_t ebp;		// Base pointer
	uint32_t eip;		// Instruction pointer
	uint32_t kstack;	// Kernel stack location
};
typedef struct arch_task arch_task_t;

typedef int task_id_t;

/* Definition of the file descriptor table structure */
struct fd_table {
	size_t len;		// Length of this table
	size_t ref_count;	// Reference count of this table
	struct vfs_node **nodes;// Pointer to the VFS nodes
};
typedef struct fd_table fd_table_t;

/* Definition of the task structure */
struct task {
	struct task *next;	// Next task
	struct priv priv;	// System privileges structure
	struct pd *page_dir;	// Page directory
	task_id_t id;		// Task ID
	struct arch_task arch;	// Architecture task implementation
	struct fd_table *fds;	// File descriptor table
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

/* Macro that retrieve the pointer of the current process */
#define CURR_PROC	(_curr_task)

/* Pointer to the current task in the system */
extern volatile struct task *_curr_task;

void init_multitask();

void switch_context();

int fork();

int getpid();

void switch_to_user_mode();

#endif	/* __TASK_H__ */
