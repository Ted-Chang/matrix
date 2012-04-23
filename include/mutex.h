#ifndef __MUTEX_H__
#define __MUTEX_H__

#include "atomic.h"
#include "hal/spinlock.h"

struct mutex {
	atomic_t value;
	uint32_t flags;
	struct spinlock lock;
	struct list threads;
	const char *name;
};

extern void mutex_init(struct mutex *lock, const char *name, uint32_t flags);
extern void mutex_acquire(struct mutex *lock);
extern void mutex_release(struct mutex *lock);

#endif	/* __MUTEX_H__ */
