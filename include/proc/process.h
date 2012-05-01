#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "status.h"

typedef int process_id_t;

struct va_space;

/* Definition of the task structure */
struct process {
	/* Main thread information */
	struct mutex lock;

	/* Scheduling information */
	int flags;
	int priority;
	struct va_space *aspace;

	/* Resource information */
	struct list threads;

	/* State of the process */
	enum {
		PROCESS_RUNNING,
		PROCESS_DEAD,
	} state;

	/* ID of the process */
	process_id_t id;
	char *name;
	int status;
};

extern struct process *_kernel_proc;

extern void init_process();

#endif	/* __PROCESS_H__ */
