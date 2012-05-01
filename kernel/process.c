/*
 * process.c
 */

#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/matrix.h"
#include "mutex.h"
#include "hal/hal.h"
#include "hal/lirq.h"
#include "matrix/debug.h"
#include "mm/kmem.h"
#include "proc/process.h"

/* Process containing all kernel-mode threads */
struct process *_kernel_proc = NULL;

static void process_ctor(void *obj)
{
	struct process *proc = (struct process *)obj;

	mutex_init(&proc->lock, "process_lock", 0);
	LIST_INIT(&proc->threads);
}

static status_t process_alloc(const char *name, int flags, int priority,
			      struct va_space *aspace, struct process *parent,
			      struct process **procp)
{
	status_t rc;
	struct process *proc;

	/* Allocate a new process structure */
	proc = kmem_alloc(sizeof(struct process));

	/* Attempt to allocate a process ID. If creating the kernel process,
	 * always give it an ID of 0.
	 */
	if (_kernel_proc) {
		;
	} else {
		proc->id = 0;
	}

	proc->flags = flags;
	proc->priority = priority;
	proc->aspace = aspace;
	proc->state = PROCESS_RUNNING;
	proc->status = 0;

	*procp = proc;

	return STATUS_SUCCESS;
}

/**
 * Start our kernel at top half
 */
void init_process()
{
}
