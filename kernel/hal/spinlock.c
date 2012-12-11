#include <types.h>
#include "matrix/matrix.h"
#include "atomic.h"
#include "barrier.h"
#include "hal/hal.h"
#include "hal/spinlock.h"
#include "debug.h"

static INLINE void spinlock_lock_internal(struct spinlock *lock)
{
	/* Attempt to take the lock. */
	if (atomic_dec(&lock->value) != 1) {
		/* When running on a UP system we don't need to spin as there should
		 * only be on thing at any time, so just die.
		 */
		PANIC("spinlock_lock_internal: lock value invalid.");
	}
}

/**
 * Acquire a spinlock
 */
void spinlock_acquire(struct spinlock *lock)
{
	boolean_t state;

	/* Disable interrupts while locked to ensure that nothing else will
	 * run on the current CPU for the duration of the lock.
	 */
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
	lock->value = 1;
	irq_restore(state);
}

void spinlock_acquire_noirq(struct spinlock *lock)
{
	ASSERT(!irq_state());

	spinlock_lock_internal(lock);

	enter_cs_barrier();
}

void spinlock_release_noirq(struct spinlock *lock)
{
	if (!spinlock_held(lock)) {
		PANIC("spinlock_release_noirq: release a lock not held.");
	}

	leave_cs_barrier();
	lock->value = 1;
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
