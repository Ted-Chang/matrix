#include <types.h>
#include <stddef.h>
#include "list.h"
#include "matrix/debug.h"

void list_test()
{
	struct list header;
	struct list entry;

	LIST_INIT(&header);
	kprintf("header:0x%x, prev:0x%x, next:0x%x\n",
		&header, header.prev, header.next);
	ASSERT(LIST_EMPTY(&header));

	list_add_tail(&entry, &header);
}
