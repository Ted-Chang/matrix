#include <types.h>
#include "matrix/matrix.h"
#include "atomic.h"
#include "lirq.h"
#include "barrier.h"
#include "matrix/debug.h"
#include "spinlock.h"

static INLINE void spinlock_lock_internal(struct spinlock *lock)
{
	if (atomic_dec(&lock->value) != 1) {
		PANIC("spinlock_lock_internal: lock value invalid.");
	}
}

/**
 * Acquire a spinlock
 */
void spinlock_acquire(struct spinlock *lock)
{
	boolean_t state;

	state = irq_disable();

	spinlock_lock_internal(lock);
	lock->state = state;
	enter_cs_barrier();
}

/**
 * Release a spinlock
 */
void spinlock_release(struct spinlock *lock)
{
	boolean_t state;

	if (!spinlock_held(lock)) {
		PANIC("spinlock_release: release a lock not held.");
	}

	/* Save state before unlocking in case it is overwritten by another
	 * waiting CPU.
	 */
	state = lock->state;

	leave_cs_barrier();
	atomic_set(&lock->value, 1);
	irq_restore(state);
}

/**
 * Initialize a spinlock
 */
void spinlock_init(struct spinlock *lock, const char *name)
{
	lock->value = 1;
	lock->name = name;
	lock->state = FALSE;
}
