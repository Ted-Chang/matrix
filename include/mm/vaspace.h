#ifndef __VASPACE_H__
#define __VASPACE_H__

#include "mm/mmu.h"
#include "cpu.h"
#include "mutex.h"

/* Structure describing a virtual address space */
struct va_space {
	struct mutex lock;		// Lock to protect the virtual address space
	
	/* Underlying MMU context for address space */
	struct mmu_ctx *mmu;
};

#define CURR_SPACE	(CURR_CPU->space)

extern void va_space_switch(struct va_space *space);
extern struct va_space *va_space_create();
extern struct va_space *va_space_clone(struct va_space *src);
extern void va_space_destroy(struct va_space *space);
extern void init_va();

#endif	/* __VASPACE_H__ */
