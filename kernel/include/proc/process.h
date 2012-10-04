#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <sys/time.h>
#include "matrix/const.h"
#include "priv.h"
#include "fs.h"
#include "fd.h"			// File descriptors

/* Our kernel stack size is 8192 bytes */
#define KSTACK_SIZE	0x2000
/* Our user stack size is 65536 bytes */
#define USTACK_SIZE	0x10000
/* Bottom of the user stack */
#define USTACK_BOTTOM	0x10000000

/* Definition of the architecture specific process structure */
struct arch_process {
	uint32_t esp;		// Stack pointer
	uint32_t ebp;		// Base pointer
	uint32_t eip;		// Instruction pointer
	uint32_t kstack;	// Kernel stack location
	uint32_t ustack;	// User stack location
	uint32_t entry;		// Entry point of the image
	size_t size;		// Total size of the image
};
typedef struct arch_process arch_process_t;

typedef int pid_t;

/* Forward declaration, used to pass arguments */
struct process_create;

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

	/* Registers at interrupt */
	struct registers *syscall_regs;
	
	clock_t usr_time;	// User time in ticks
	clock_t sys_time;	// System time in ticks
	int8_t priority;	// Current scheduling priority
	int8_t max_priority;	// Max priority of the process
	int8_t ticks_left;	// Number of scheduling ticks left
	int8_t quantum;		// Quantum in ticks
	char name[P_NAME_LEN];	// Name of the process, include `\0'

	/* State of the process */
	enum {
		PROCESS_RUNNING,
		PROCESS_DEAD
	} state;
	
	struct process_create *create;// Internal creation info structure
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

void switch_context();
int fork();
int getpid();
void switch_to_user_mode(uint32_t location, uint32_t ustack);
int exec(char *path, int argc, char **argv);
int system(char *path, int argc, char **argv);
void init_process();

#endif	/* __PROCESS_H__ */
