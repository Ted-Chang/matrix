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
#include "idmgr.h"

/* Process ID manager */
static struct id_mgr _process_id_mgr;

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
		proc->id = id_mgr_alloc(&_process_id_mgr);
		if (proc->id < 0) {
			return STATUS_PROCESS_LIMIT;
		}
	} else {
		proc->id = 0;
	}

	proc->flags = flags;
	proc->priority = priority;
	proc->state = PROCESS_RUNNING;
	
	proc->name = kmem_alloc(strlen(name) + 1);
	strcpy(proc->name, name);

	proc->aspace = aspace;
	proc->state = PROCESS_RUNNING;
	proc->status = 0;

	DEBUG(DL_DBG, ("Process(%s) created at address(%p), id(%d)\n",
		       proc->name, proc, proc->id));
	
	*procp = proc;

	return STATUS_SUCCESS;
}

void process_attach(struct process *p, struct thread *t)
{
	t->owner = p;

	mutex_acquire(&p->lock);

	ASSERT(p->state != PROCESS_DEAD);
	list_add_tail(&t->owner_link, &p->threads);
	
	mutex_release(&p->lock);
}

void process_detach(struct thread *t)
{
	;
}

/**
 * Start our kernel at top half
 */
void init_process()
{
	status_t rc;
	
	/* Initialize the process ID manager. We reserve ID 0 as the kernel
	 * process ID
	 */
	id_mgr_init(&_process_id_mgr, 65535, 0);
	id_mgr_reserve(&_process_id_mgr, 0);

	/* Create the system process */
	rc = process_alloc("system", 0, 0, NULL, NULL, &_kernel_proc);
	if (rc != STATUS_SUCCESS) {
		PANIC("Create system process failed!");
	}
}
