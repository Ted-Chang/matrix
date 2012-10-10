#ifndef __KMEM_H__
#define __KMEM_H__

#include <stddef.h>

#define KHEAP_START		0xC0000000
#define KHEAP_INITIAL_SIZE	0x200000

#define HEAP_INDEX_SIZE		0x20000
#define HEAP_MAGIC		0x123890AB
#define HEAP_MIN_SIZE		0x70000


/*
 * Routines for allocate a chunk of memory
 */
void *kmem_alloc(size_t size, int mmflag);
void *kmem_alloc_p(size_t size, uint32_t *phys, int mmflag);
void kmem_free(void *p);
void init_kmem();

#endif	/* __KMEM_H__ */
