#include <types.h>
#include <stddef.h>
#include "atomic.h"
#include "list.h"
#include "hal/spinlock.h"
#include "mutex.h"
#include "status.h"
#include "matrix/debug.h"

static INLINE status_t mutex_acquire_internal(struct mutex *lock,
					      uint64_t timeout, int flags)
{
	status_t rc;

	if (!atomic_tas(&lock->value, 0, 1)) {
		;
	}

	return STATUS_SUCCESS;
}

void mutex_acquire(struct mutex *lock)
{
	mutex_acquire_internal(lock, -1, 0);
}

void mutex_release(struct mutex *lock)
{
	spinlock_acquire(&lock->lock);

	if (!atomic_get(&lock->value))
		PANIC("Release a mutex which no one held it");

	if (atomic_get(&lock->value) == 1) {
		if (!LIST_EMPTY(&lock->threads)) {
			;
		} else {
			atomic_dec(&lock->value);
		}
	} else 
		atomic_dec(&lock->value);
	
	spinlock_release(&lock->lock);
}

void mutex_init(struct mutex *lock, const char *name, uint32_t flags)
{
	atomic_set(&lock->value, 0);
	spinlock_init(&lock->lock, "mutex_lock");
	LIST_INIT(&lock->threads);
	lock->flags = flags;
	lock->name = name;
}

