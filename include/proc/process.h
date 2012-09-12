#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <sys/time.h>
#include "matrix/const.h"
#include "priv.h"
#include "fs.h"
#include "fd.h"			// File descriptors

/* Our kernel stack size is 4096 bytes */
#define KSTACK_SIZE 4096

typedef uint32_t uid_t;
typedef uint32_t gid_t;

/* Definition of the architecture specific process structure */
struct arch_process {
	uint32_t esp;		// Stack pointer
	uint32_t ebp;		// Base pointer
	uint32_t eip;		// Instruction pointer
	uint32_t kstack;	// Kernel stack location
};
typedef struct arch_process arch_process_t;

typedef int pid_t;

/* Definition of the process structure */
struct process {
	struct process *next;	// Next process
	struct priv priv;	// System privileges structure
	struct mmu_ctx *mmu_ctx;// MMU context
	pid_t id;		// Process ID
	uid_t uid;		// User ID
	gid_t gid;		// Group ID
	struct arch_process arch;// Architecture process implementation
	struct fd_table *fds;	// File descriptor table
	clock_t usr_time;	// User time in ticks
	clock_t sys_time;	// System time in ticks
	int8_t priority;	// Current scheduling priority
	int8_t max_priority;	// Max priority of the process
	int8_t ticks_left;	// Number of scheduling ticks left
	int8_t quantum;		// Quantum in ticks
	char name[P_NAME_LEN];	// Name of the process, include `\0'
};
typedef struct process process_t;

/* Scheduling priority for our processes. Value must start at zero (highest
 * priority) and increment.
 */
#define NR_SCHED_QUEUES		16
#define PROCESS_Q		0
#define MAX_USER_Q		0
#define USER_Q			7
#define MIN_USER_Q		14
#define IDLE_Q			15

/* Pointer to the current process in the system */
extern volatile struct process *_curr_proc;

/* Macro that retrieve the pointer of the current process */
#define CURR_PROC	(_curr_proc)

void init_process();
void switch_context();
int fork();
int getpid();
void switch_to_user_mode();

#endif	/* __PROCESS_H__ */
