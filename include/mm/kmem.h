#ifndef __KMEM_H__
#define __KMEM_H__

extern void *kmem_alloc(size_t size);
extern void *kmem_alloc_a(size_t size);
extern void *kmem_alloc_p(size_t size, uint32_t *phys);
extern void *kmem_alloc_ap(size_t size, uint32_t *phys);
extern void *kmem_alloc_int(size_t size, int align, uint32_t *phys);
extern void kmem_free(void *p);
extern void *kmem_map(phys_addr_t base, size_t size, int mmflag);

extern void init_kmem();

#endif	/* __KMEM_H__ */
