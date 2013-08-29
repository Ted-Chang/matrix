#ifndef __VA_H__
#define __VA_H__

#include "mm/mmu.h"

struct va_space {
	struct mmu_ctx *mmu;
};

/* Map flags for va_map */
#define VA_MAP_READ	(1<<0)
#define VA_MAP_WRITE	(1<<1)
#define VA_MAP_EXEC	(1<<2)
#define VA_MAP_FIXED	(1<<3)

extern struct va_space *va_create();
extern void va_destroy(struct va_space *vas);
extern int va_map(struct va_space *vas, ptr_t start, size_t size, int flags, ptr_t *addrp);
extern int va_unmap(struct va_space *vas, ptr_t start, size_t size);
extern void va_switch(struct va_space *vas);
extern void init_va();

#endif	/* __VA_H__ */
