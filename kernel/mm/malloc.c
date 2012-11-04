#include <string.h>
#include "mm/malloc.h"
#include "mm/kmem.h"

/* Information structure prepended to allocations */
typedef struct alloc_tag {
	size_t size;	// Size of the allocation
	void *cache;	// Pointer to the cache for allocation
} alloc_tag_t;

void *kmalloc(size_t size, int mmflag)
{
	void *addr;

	addr = kmem_alloc(size, mmflag);
	if (!addr) {
		return NULL;
	}
	
	if (mmflag & MM_ZERO_F) {
		memset(addr, 0, size);
	}

	return addr;
}

void *kcalloc(size_t nmemb, size_t size, int mmflag)
{
	return kmalloc(nmemb * size, mmflag | MM_ZERO_F);
}

void *krealloc(void *addr, size_t size, int mmflag)
{
	void *mem;

	if (!addr) {
		return kmalloc(size, mmflag);
	}

	/* Make a new allocation */
	mem = kmalloc(size, mmflag & ~MM_ZERO_F);
	if (!mem) {
		return mem;
	}

	/* Copy the block data using the smallest of the two sizes */
	//memcpy(mem, addr, MIN());

	/* Zero any new space if needed */

	/* Free the original allocation */
	kfree(addr);

	return mem;
}

void kfree(void *addr)
{
	if (addr) {
		kmem_free(addr);
	}
}

/* Initialize the allocator caches */
void init_malloc()
{
	;
}
