#include <string.h>
#include "mm/malloc.h"
#include "mm/kmem.h"

void *kmalloc(size_t size, int mmflag)
{
	void *addr;

	addr = kmem_alloc(size, mmflag);
	if (!addr) {
		return NULL;
	}
	
	if (mmflag & MM_ZERO) {
		memset(addr, 0, size);
	}

	return addr;
}

void *kcalloc(size_t nmemb, size_t size, int mmflag)
{
	return kmalloc(nmemb * size, mmflag | MM_ZERO);
}

void *krealloc(void *addr, size_t size, int mmflag)
{
	void *mem;

	if (!addr) {
		return kmalloc(size, mmflag);
	}

	/* Make a new allocation */
	mem = kmalloc(size, mmflag & ~MM_ZERO);
	if (!mem) {
		return mem;
	}

	/* Copy the block data using the smallest of the two sizes */
	//...

	/* Zero any new space if needed */
	//...

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
