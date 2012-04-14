#ifndef __KMEM_H__
#define __KMEM_H__

extern void *kmem_map(phys_addr_t base, size_t size, int mmflag);
extern void kmem_unmap(void *addr, size_t size, boolean_t shared);

extern void init_kmem();

#endif	/* __KMEM_H__ */
