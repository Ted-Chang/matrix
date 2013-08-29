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

/* Determine if an MMU context is the kernel context */
#define IS_KERNEL_CTX(ctx)	(ctx == &_kernel_mmu_ctx)

extern void page_fault(struct registers *regs);
extern struct mmu_ctx *mmu_create_ctx();
extern struct page *mmu_get_page(struct mmu_ctx *ctx, ptr_t addr, boolean_t make, int mmflag);
extern int mmu_map(struct mmu_ctx *ctx, ptr_t virt, phys_addr_t phys, int flags);
extern int mmu_unmap(struct mmu_ctx *ctx, ptr_t virt, boolean_t shared, phys_addr_t *physp);
extern void mmu_load_ctx(struct mmu_ctx *ctx);
extern void mmu_clone_ctx(struct mmu_ctx *dst, struct mmu_ctx *src);
extern void mmu_destroy_ctx(struct mmu_ctx *ctx);
extern void init_mmu();

#endif	/* __MMU_H__ */
