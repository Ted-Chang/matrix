/*
 * mmgr.h
 */

#ifndef __MMGR_H__
#define __MMGR_H__

#include <isr.h>	// For struct registers
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
extern struct mmu_ctx *_current_mmu_ctx;

void page_fault(struct registers *regs);
struct mmu_ctx *mmu_create_ctx();
void mmu_destroy_ctx(struct mmu_ctx *ctx);
struct page *mmu_get_page(struct mmu_ctx *ctx, uint32_t addr,
			  boolean_t make, int mmflag);
int mmu_map_page(struct mmu_ctx *ctx, uint32_t virt, uint32_t phys,
		 boolean_t write, boolean_t execute, int mmflag);
int mmu_unmap_page(struct mmu_ctx *ctx, uint32_t virt, boolean_t shared,
		   uint32_t *phys);
void mmu_switch_ctx(struct mmu_ctx *ctx);
void init_mmu();

#endif	/* __MMU_H__ */
