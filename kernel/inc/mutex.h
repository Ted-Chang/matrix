#ifndef __MUTEX_H__
#define __MUTEX_H__

#include "hal/spinlock.h"
#include "atomic.h"
#include "list.h"

/* Forward declaration of thread */
struct thread;

/* Structure containing a mutex */
struct mutex {
	atomic_t value;		// Lock count
	int flags;		// Behaviour flags for the mutex
	struct spinlock lock;	// Lock to protect the thread list
	struct list threads;	// List of waiting threads
	struct thread *owner;	// Owner of the lock
	const char *name;	// Name of the mutex
};
typedef struct mutex mutex_t;

static INLINE boolean_t mutex_held(struct mutex *m) {
	return m->value != 0;
}

extern void mutex_acquire(struct mutex *m);
extern void mutex_release(struct mutex *m);
extern void mutex_init(struct mutex *m, const char *name, int flags);

#endif	/* __MUTEX_H__ */
