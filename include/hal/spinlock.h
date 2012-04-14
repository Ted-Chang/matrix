#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include "atomic.h"
#include "matrix/matrix.h"

struct spinlock {
	atomic_t value;
	volatile boolean_t state;
	const char *name;
};
typedef struct spinlock spinlock_t;


static INLINE boolean_t spinlock_held(struct spinlock *lock)
{
	return lock->value != 1;
}

void spinlock_init(struct spinlock *lock, const char *name);
void spinlock_acquire(struct spinlock *lock);
void spinlock_release(struct spinlock *lock);

#endif	/* __SPINLOCK_H__ */
