#include <types.h>
#include "spinlock.h"

struct page_queue {
	struct list pages;		// List of pages
	struct spinlock lock;		// Lock to protect the queue
};

void init_page()
{
	uint64_t pages_base = 0, offset = 0, addr;
	uint64_t pages_size = 0, size;

	/* Initialize page queues and freelists */
}
