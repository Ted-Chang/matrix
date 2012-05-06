#include <types.h>
#include <stddef.h>
#include "status.h"
#include "list.h"
#include "mutex.h"
#include "idmgr.h"
#include "matrix/debug.h"
#include "mm/mlayout.h"
#include "mm/kmem.h"
#include "proc/process.h"
#include "proc/thread.h"

/* Thread ID manager */
static struct id_mgr _thread_id_mgr;

void init_arch_thread(struct thread *t, void *stack)
{
	t->arch.flags = 0;

	/* Point the stack pointer for system call entry at the top of the stack */
	t->arch.kernel_stack = (uint32_t)((char *)stack + KSTK_SIZE);
}

static void thread_ctor(void *obj)
{
	;
}

void run_thread(struct thread *t)
{
	spinlock_acquire(&t->lock);
	
	ASSERT(t->state == THREAD_CREATED);
	t->state = THREAD_READY;

	spinlock_release(&t->lock);
}

status_t create_thread(struct process *owner, uint32_t flags,
		       thread_func_t func, void *arg,
		       struct thread **threadp)
{
	status_t rc;
	struct thread *t;

	if (!owner)
		owner = _kernel_proc;

	/* Allocate the thread structure */
	t = kmem_alloc(sizeof(struct thread));
	t->id = id_mgr_alloc(&_thread_id_mgr);
	if (t->id < 0) {
		kmem_free(t);
		return STATUS_THREAD_LIMIT;
	}

	/* Allocate a kernel stack and initialize the thread context */
	t->kstack = kmem_alloc(KSTK_SIZE);

	/* Initialize the architecture-specific data */
	init_arch_thread(t, t->kstack);

	/* Initially set the CPU pointer to NULL, the thread will be assigned to a
	 * CPU when run_thread() is called on it.
	 */
	t->cpu = NULL;

	t->state = THREAD_CREATED;
	t->flags = flags;
	t->priority = THREAD_PRIORITY_NORMAL;
	t->max_priority = -1;
	t->curr_priority = -1;
	t->timeslice = 0;
	t->func = func;
	t->arg = arg;
	t->ustack = 0;
	t->ustack_size = 0;

	/* Attach the thread to the owner */
	process_attach(owner, t);

	DEBUG(DL_DBG, ("thread created at %p, thread id(%d), owner(%p)\n",
		       t, t->id, owner));

	if (threadp)
		*threadp = t;
	else
		run_thread(t);	// Caller doesn't want a pointer, just start it

	return STATUS_SUCCESS;
}

void init_thread()
{
	/* Initialize the thread ID manager */
	id_mgr_init(&_thread_id_mgr, 65535, 0);
}
