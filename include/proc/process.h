#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "status.h"
#include "proc/thread.h"

typedef int process_id_t;

struct va_space;

/* Definition of the task structure */
struct process {
	/* Main thread information */
	struct mutex lock;

	/* Scheduling information */
	int flags;
	int priority;
	struct va_space *aspace;	// Virtual address space for the process

	/* Resource information */
	struct list threads;		// List of threads

	/* State of the process */
	enum {
		PROCESS_RUNNING,
		PROCESS_DEAD,
	} state;

	/* ID of the process */
	process_id_t id;
	char *name;
	int status;			// Exit status
};

/* Macro that expands to a pointer to the current process */
#define CURR_PROC	(CURR_THREAD->owner)

extern struct process *_kernel_proc;

extern void process_attach(struct process *p, struct thread *t);
extern void process_detach(struct thread *t);

extern void init_process();

#endif	/* __PROCESS_H__ */
