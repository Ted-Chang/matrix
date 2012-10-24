#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <types.h>
#include "sys/time.h"
#include "matrix/const.h"
#include "matrix/matrix.h"
#include "list.h"
#include "fs.h"
#include "fd.h"			// File descriptors
#include "avltree.h"
#include "notifier.h"
#include "mm/mlayout.h"		// For memory layout
#include "proc/thread.h"

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

	/* Per process pointer store the regs on stack of this process */
	struct registers *syscall_regs;
};
typedef struct arch_process arch_process_t;

/* Forward declaration, used to pass arguments */
struct process_create;

/* Definition of the process structure */
struct process {
	struct list link;		// Link to next process

	int ref_count;			// Number of handles/threads in the process

	struct mmu_ctx *mmu_ctx;	// MMU context

	pid_t id;			// Process ID
	uid_t uid;			// User ID
	gid_t gid;			// Group ID
	struct arch_process arch;	// Architecture process implementation
	struct fd_table *fds;		// File descriptor table

	int8_t priority;		// Current scheduling priority
	int8_t max_priority;		// Max priority of the process
	int8_t ticks_left;		// Number of scheduling ticks left
	int8_t quantum;			// Quantum in ticks
	char name[P_NAME_LEN];		// Name of the process, include `\0'

	struct list threads;		// List of threads

	/* State of the process */
	enum {
		PROCESS_RUNNING,
		PROCESS_DEAD
	} state;

	/* Other process information */
	struct avl_tree_node tree_link;	// Link to the process tree

	struct notifier death_notifier;	// Notifier list of this process
	int status;			// Exit status
	
	struct process_create *create;	// Internal creation info structure
};
typedef struct process process_t;

/* Pointer to the current process in the system */
extern struct process *_curr_proc;

/* Macro that retrieve the pointer of the current process */
#define CURR_PROC	(_curr_proc)

/* Pointer to the kernel process */
extern struct process *_kernel_proc;

extern int fork();
extern int getpid();
extern void switch_to_user_mode(uint32_t location, uint32_t ustack);
extern int exec(char *path, int argc, char **argv);
extern int system(char *path, int argc, char **argv);
extern struct process *process_lookup(pid_t pid);
extern void process_attach(struct process *p, struct thread *t);
extern void process_detach(struct thread *t);
extern void process_switch(struct process *curr, struct process *prev);
extern int process_wait(struct process *p);
extern void init_process();

#endif	/* __PROCESS_H__ */
