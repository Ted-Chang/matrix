#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "mm/malloc.h"
#include "mm/slab.h"
#include "proc/process.h"
#include "proc/thread.h"
#include "proc/sched.h"

static void thread_ctor(void *obj)
{
	struct thread *t = (struct thread *)obj;

	LIST_INIT(&t->runq_link);
}

void init_thread()
{
	;
}


