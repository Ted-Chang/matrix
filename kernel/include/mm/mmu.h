/*
 * mmgr.h
 */

#ifndef __MMGR_H__
#define __MMGR_H__

#include "hal/isr.h"	// For struct registers
#include "page.h"

struct pdir;

/*
 * MMU context
 */
struct mmu_ctx {
	/* Virtual address of the page directory */
	struct pdir *pdir;

	/* Physical address of the page directory */
	uint32_t pdbr;
};

extern struct mmu_ctx _kernel_mmu_ctx;

/* Macro that expands to a pointer to the current address space */
#define CURR_ASPACE	(CURR_CPU->aspace)

extern void page_fault(struct registers *regs);
extern struct mmu_ctx *mmu_create_ctx();
extern void mmu_destroy_ctx(struct mmu_ctx *ctx);
extern struct page *mmu_get_page(struct mmu_ctx *ctx, uint32_t addr,
				 boolean_t make, int mmflag);
extern int mmu_map_page(struct mmu_ctx *ctx, uint32_t virt, uint32_t phys,
			boolean_t write, boolean_t execute, int mmflag);
extern int mmu_unmap_page(struct mmu_ctx *ctx, uint32_t virt, boolean_t shared,
			  uint32_t *phys);
extern void mmu_switch_ctx(struct mmu_ctx *ctx);
extern void mmu_copy_ctx(struct mmu_ctx *dst, struct mmu_ctx *src);
extern void init_mmu();

#endif	/* __MMU_H__ */
