#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <types.h>
#include "sys/time.h"
#include "matrix/const.h"
#include "matrix/matrix.h"
#include "list.h"
#include "avltree.h"
#include "notifier.h"
#include "mm/mlayout.h"		// For memory layout
#include "proc/thread.h"
#include "fs.h"
#include "fd.h"			// File descriptors

/* Bottom of the user stack */
#define USTACK_BOTTOM	0x30000000

/* Forward declaration, used to pass arguments */
struct process_create;

/* Definition of the process structure */
struct process {
	int ref_count;			// Number of handles/threads in the process

	struct mmu_ctx *mmu_ctx;	// MMU context

	pid_t id;			// Process ID
	uid_t uid;			// User ID
	gid_t gid;			// Group ID
	
	struct fd_table *fds;		// File descriptor table

	int8_t priority;		// Current scheduling priority
	char name[P_NAME_LEN];		// Name of the process, include `\0'

	struct list threads;		// List of threads

	/* State of the process */
	enum {
		PROCESS_RUNNING,
		PROCESS_DEAD,
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
extern void process_exit(int status);
extern int process_wait(struct process *p);
extern int process_create(const char *args[], struct process *parent, int flags,
			  int priority, struct process **procp);
extern int process_destroy(struct process *proc);
extern void init_process();

#endif	/* __PROCESS_H__ */
