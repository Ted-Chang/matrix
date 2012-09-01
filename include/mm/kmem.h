#ifndef __KMEM_H__
#define __KMEM_H__

#include <stddef.h>

#define KHEAP_START		0xC0000000
#define KHEAP_INITIAL_SIZE	0x100000

#define HEAP_INDEX_SIZE		0x20000
#define HEAP_MAGIC		0x123890AB
#define HEAP_MIN_SIZE		0x70000


/*
 * Routines for allocate a chunk of memory
 */
void *kmalloc(size_t size);
void *kmalloc_a(size_t size);
void *kmalloc_p(size_t size, uint32_t *phys);
void *kmalloc_ap(size_t size, uint32_t *phys);
void *kmalloc_int(size_t size, int align, uint32_t *phys);
void kfree(void *p);

#endif	/* __KMEM_H__ */
