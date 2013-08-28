#ifndef __VA_H__
#define __VA_H__

#include "mm/mmu.h"

struct va_space {
	struct mmu_ctx *mmu;
};

extern struct va_space *va_create();
extern void va_destroy(struct va_space *vas);
extern void init_va();

#endif	/* __VA_H__ */
