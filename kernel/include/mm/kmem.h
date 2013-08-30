#ifndef __KMEM_H__
#define __KMEM_H__

#include <stddef.h>

#define POOL_INDEX_SIZE		0x20000
#define POOL_MAGIC		0x123890AB
#define POOL_MIN_SIZE		0x70000


/*
 * Routines for allocate a chunk of memory
 */
extern void *kmem_alloc(size_t size, int mmflag);
extern void *kmem_alloc_p(size_t size, uint32_t *phys, int mmflag);
extern void kmem_free(void *p);
extern void *kmem_map(phys_addr_t base, size_t size, int mmflag);
extern void kmem_unmap(void *addr, size_t size, boolean_t shared);
extern void init_kmem();

#endif	/* __KMEM_H__ */
