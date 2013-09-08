#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <types.h>
#include "sys/time.h"
#include "matrix/const.h"
#include "matrix/matrix.h"
#include "list.h"
#include "rtl/avltree.h"
#include "rtl/notifier.h"
#include "proc/thread.h"
#include "fs.h"
#include "fd.h"			// File descriptors
#include "ioctx.h"

/* Bottom of the user stack */
#define USTACK_BOTTOM	0x30000000

/* Forward declaration, used to pass arguments */
struct process_creation;

/* Definition of the process structure */
struct process {
	int ref_count;				// Number of handles/threads in the process

	struct va_space *vas;			// Virtual address space

	pid_t id;				// Process ID
	uid_t uid;				// User ID
	gid_t gid;				// Group ID
	
	int flags;				// Behaviour flags for the process
	int8_t priority;			// Current scheduling priority
	char *name;				// Name of the process

	struct list threads;			// List of threads

	/* Signal information */
	sigset_t signal_mask;			// Bitmap of masked signals
	struct sigaction signal_act[NSIG];

	/* State of the process */
	enum {
		PROCESS_RUNNING,
		PROCESS_DEAD,
	} state;

	struct fd_table *fds;			// File descriptor table
	struct io_ctx ioctx;			// IO context

	/* Other process information */
	struct avl_tree_node tree_link;		// Link to the process tree

	struct notifier death_notifier;		// Notifier list of this process

	int status;				// Exit status
	
	struct process_creation *creation;	// Internal creation info structure
};
typedef struct process process_t;

/* Macro that retrieve the pointer of the current process */
#define CURR_PROC	(CURR_THREAD->owner)

/* Internal flags for process creation */
#define PROCESS_CLONE_F		(1<<0)

/* Pointer to the kernel process */
extern struct process *_kernel_proc;

extern struct process *process_lookup(pid_t pid);

extern void process_attach(struct process *p, struct thread *t);
extern void process_detach(struct thread *t);

extern int process_exit(int status);
extern int process_create(const char **args, struct process *parent, int flags,
			  int priority, struct process **procp);
extern int process_destroy(struct process *proc);

extern int process_wait(struct process *p, void *sync);
extern int process_getid();

extern void init_process();
extern void shutdown_process();

#endif	/* __PROCESS_H__ */
