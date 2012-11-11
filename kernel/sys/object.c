#include <stdio.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "semaphore.h"
#include "object.h"

/* Object waiting internal data structure */
struct object_wait_sync {
	struct semaphore *sem;		// Pointer to the semaphore
	boolean_t signalled;
};

void object_wait_signal(void *sync)
{
	struct object_wait_sync *s;

	s = (struct object_wait_sync *)sync;
	s->signalled = TRUE;
	semaphore_up(s->sem, 1);
}

void object_wait_notifier(void *data)
{
	object_wait_signal(data);
}
