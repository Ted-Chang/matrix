#include <types.h>
#include <stddef.h>
#include "list.h"
#include "cpu.h"
#include "mm/mlayout.h"
#include "mm/mm.h"
#include "mm/page.h"
#include "mm/mmu.h"

/* Kernel MMU context */
struct mmu_ctx _kernel_mmu_ctx;

/* Allocate a page */
static phys_addr_t alloc_structure(int mmflag)
{
	struct page *page = page_alloc(mmflag | MM_ZERO);
	return (page) ? page->addr : 0;
}

static uint32_t *mmu_ctx_get_pdir(struct mmu_ctx *ctx, void *virt, boolean_t alloc, int mmflag)
{
	uint64_t *pml4, *pdp;
	uint32_t pml4e, pdpe;
	phys_addr_t page;

	return NULL;
}

void mmu_lock_ctx(struct mmu_ctx *ctx)
{
	;
}

void mmu_unlock_ctx(struct mmu_ctx *ctx)
{
	;
}

void mmu_switch_ctx(struct mmu_ctx *ctx)
{
	x86_write_cr3(ctx->pml4);
}

void arch_init_mmu()
{
	phys_addr_t i, j, highest_phys = 0;
	uint64_t *pdir;
	
	/* Initialize the kernel MMU context structure */
	mutex_init(&_kernel_mmu_ctx.lock, "mmu_ctx_lock", 0);
	_kernel_mmu_ctx.invalidate_count = 0;
	_kernel_mmu_ctx.pml4 = alloc_structure(MM_BOOT);

	mmu_lock_ctx(&_kernel_mmu_ctx);

	/* Create the physical map area */
	for (i = 0; i < highest_phys; i += 0x40000000) {
		pdir = mmu_ctx_get_pdir(&_kernel_mmu_ctx,
					i + KERNEL_PMAP_BASE, TRUE, MM_BOOT);
	}

	mmu_unlock_ctx(&_kernel_mmu_ctx);
}

void arch_init_mmu_per_cpu()
{
	/* As I do not plan to support the PAT (Page Attribute Tables) 
	 * feature in this version, so I do nothing here
	 */
}

void init_mmu()
{
	arch_init_mmu();
}

void init_mmu_per_cpu()
{
	/* Do architecture specific initialization */
	arch_init_mmu_per_cpu();

	/* Switch to the kernel context */
	mmu_switch_ctx(&_kernel_mmu_ctx);
}
