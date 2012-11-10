#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "proc/thread.h"
#include "mutex.h"

static int mutex_acquire_internal(struct mutex *m, useconds_t timeout, int flags)
{
	int rc;

	if (!atomic_tas(&m->value, 0, 1)) {
		if (m->owner == CURR_THREAD) {
			;
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
	spinlock_acquire(&m->lock);

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
