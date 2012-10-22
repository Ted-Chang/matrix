#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "mm/kmem.h"
#include "mm/malloc.h"
#include "mm/slab.h"
#include "proc/thread.h"
#include "proc/process.h"
#include "proc/sched.h"

/* Thread structure cache */
static struct slab_cache *_thread_cache;

static void thread_ctor(void *obj, void *data)
{
	struct thread *t = (struct thread *)obj;

	t->ref_count = 0;
	LIST_INIT(&t->runq_link);
}

static void thread_dtor(void *obj)
{
	;
}

int thread_create(struct process *owner, int flags, thread_func_t func, void *args)
{
	int rc;
	struct thread *t;

	if (!owner) {
		owner = _kernel_proc;
	}
	
	t = kmalloc(sizeof(struct thread), 0);
	if (!t) {
		goto out;
	}

	t->kstack = kmem_alloc(KSTACK_SIZE, 0);

	t->cpu = NULL;

	t->state = THREAD_CREATED;
	t->flags = flags;
	t->ustack = 0;
	t->ustack_size = 0;

	/* Add the thread to the owner */
	process_attach(owner, t);

out:
	return rc;
}

void init_thread()
{
	/* Create the thread slab cache */
	_thread_cache = slab_cache_create("thread_cache", sizeof(struct thread), 0,
					  thread_ctor, NULL, NULL, 0, 0);
}


