#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "mutex.h"

static int mutex_acquire_internal(struct mutex *m, useconds_t timeout, int flags)
{
	int rc;

	if (!atomic_tas(&m->value, 0, 1)) {
		;
	}

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
