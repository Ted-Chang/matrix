#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "proc/thread.h"
#include "mutex.h"

static INLINE void mutex_recursive_error(struct mutex *m)
{
	PANIC("Recursive locking of non-recursive mutex");
}

static int mutex_acquire_internal(struct mutex *m, useconds_t timeout, int flags)
{
	int rc = -1;
	
	if (!atomic_tas(&m->value, 0, 1)) {
		if (m->owner == CURR_THREAD) {
			mutex_recursive_error(m);
		} else {
			spinlock_acquire(&m->lock);

			/* Check again now that we owned the lock, in case mutex_release()
			 * was called on another CPU
			 */
			if (atomic_tas(&m->value, 0, 1)) {
				spinlock_release(&m->lock);
			} else {
				list_add_tail(&CURR_THREAD->wait_link, &m->threads);

				/* If thread_sleep is successful, we will own the lock */
				rc = thread_sleep(&m->lock, timeout, m->name, flags);
				if (rc != 0) {
					return rc;
				}
			}
		}
	}

	m->owner = CURR_THREAD;
	return 0;
}

void mutex_acquire(struct mutex *m)
{
	mutex_acquire_internal(m, -1, 0);
}

void mutex_release(struct mutex *m)
{
	struct thread *t;
	struct list *l;
	
	spinlock_acquire(&m->lock);

	if (!m->value) {
		PANIC("Release of unowned mutex");
	} else if (m->owner != CURR_THREAD) {
		PANIC("Release of mutex from incorrect thread");
	}

	/* If the current value is 1, the mutex is being released. If there is
	 * a thread waiting, we do not need to modify the count, as we transfer
	 * ownership of the lock to it. Otherwise, decrement the count.
	 */
	if (m->value == 1) {
		m->owner = NULL;
		if (!LIST_EMPTY(&m->threads)) {
			l = m->threads.next;
			t = LIST_ENTRY(l, struct thread, wait_link);
			thread_wake(t);
		} else {
			atomic_dec(&m->value);
		}
	} else {
		atomic_dec(&m->value);
	}

	spinlock_release(&m->lock);
}

void mutex_init(struct mutex *m, const char *name, int flags)
{
	m->value = 0;
	spinlock_init(&m->lock, "mutex-lock");
	LIST_INIT(&m->threads);
	m->flags = flags;
	m->owner = NULL;
	m->name = name;
}
