/*
 * mmu.h
 */

#ifndef __MMU_H__
#define __MMU_H__

#include "hal/isr.h"	// For struct registers
#include "page.h"
#include "mutex.h"

struct pdir;

/*
 * MMU context
 */
struct mmu_ctx {
	/* Virtual address of the page directory */
	struct pdir *pdir;

	/* Physical address of the page directory */
	phys_addr_t pdbr;

	/* MMU context lock */
	struct mutex lock;
};

extern struct mmu_ctx _kernel_mmu_ctx;

/* Macro that expands to a pointer to the current address space */
#define CURR_ASPACE	(CURR_CORE->aspace)

/* Map flags for mmu_map */
#define MAP_READ_F	(1<<0)
#define MAP_WRITE_F	(1<<1)
#define MAP_EXEC_F	(1<<2)
#define MAP_FIXED_F	(1<<3)

extern void page_fault(struct registers *regs);
extern struct mmu_ctx *mmu_create_ctx();
extern struct page *mmu_get_page(struct mmu_ctx *ctx, ptr_t addr, boolean_t make, int mmflag);
extern int mmu_map(struct mmu_ctx *ctx, ptr_t start, size_t size, int flags);
extern int mmu_unmap(struct mmu_ctx *ctx, ptr_t start, size_t size);
extern void mmu_load_ctx(struct mmu_ctx *ctx);
extern void mmu_clone_ctx(struct mmu_ctx *dst, struct mmu_ctx *src);
extern void mmu_destroy_ctx(struct mmu_ctx *ctx);
extern void init_mmu();

#endif	/* __MMU_H__ */
