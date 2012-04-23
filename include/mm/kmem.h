#ifndef __KMEM_H__
#define __KMEM_H__

extern void *kmalloc(size_t size);
extern void *kmalloc_a(size_t size);
extern void *kmalloc_p(size_t size, uint32_t *phys);
extern void *kmalloc_ap(size_t size, uint32_t *phys);
extern void *kmalloc_int(size_t size, int align, uint32_t *phys);
extern void kfree(void *p);

extern void init_kmem();

#endif	/* __KMEM_H__ */
