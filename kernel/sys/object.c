#include <stdio.h>
#include <stddef.h>
#include "object.h"

/* Object waiting internal data structure */
struct object_wait_sync {
	;
};

void object_wait_signal(void *sync)
{
	;
}

void object_wait_notifier(void *data)
{
	object_wait_signal(data);
}
