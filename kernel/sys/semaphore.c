#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "proc/thread.h"
#include "semaphore.h"
#include "debug.h"

void semaphore_down(struct semaphore *s)
{
	DEBUG(DL_DBG, ("sem(%s) count(%d).\n", s->name, s->count));
	
	spinlock_acquire(&s->lock);

	if (s->count) {
		--(s->count);
		spinlock_release(&s->lock);
		return;
	}

	list_add_tail(&CURR_THREAD->wait_link, &s->threads);
	DEBUG(DL_DBG, ("sem(%s) put thread(%s:%d) to wait list.\n",
		       s->name, CURR_THREAD->name, CURR_THREAD->id));
	thread_sleep(&s->lock, -1, s->name, 0);
}

void semaphore_up(struct semaphore *s, size_t count)
{
	struct thread *t;
	struct list *l;
	size_t i;

	DEBUG(DL_DBG, ("sem(%s) count(%d).\n", s->name, s->count));
	
	spinlock_acquire(&s->lock);

	for (i = 0; i < count; i++) {
		if (LIST_EMPTY(&s->threads)) {
			s->count++;
		} else {
			l = s->threads.next;
			t = LIST_ENTRY(l, struct thread, wait_link);
			DEBUG(DL_DBG, ("sem(%s) waking up thread(%s:%d).\n",
				       s->name, t->name, t->id));
			thread_wake(t);
		}
	}

	spinlock_release(&s->lock);
}

void semaphore_init(struct semaphore *s, const char *name, size_t initial)
{
	spinlock_init(&s->lock, "sem-lock");
	LIST_INIT(&s->threads);
	s->count = initial;
	s->name = name;
}
