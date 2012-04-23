#include <types.h>
#include <stddef.h>
#include <string.h>
#include "list.h"
#include "cpu.h"
#include "mm/mlayout.h"
#include "mm/mm.h"
#include "mm/page.h"
#include "mm/mmu.h"
#include "matrix/debug.h"
#include "status.h"

/* Check if an MMU context is the kernel context */
#define IS_KERNEL_CTX(ctx)	(ctx == &_kernel_mmu_ctx)

extern void copy_page_physical(uint32_t dst, uint32_t src);

/* Kernel MMU context */
struct mmu_ctx _kernel_mmu_ctx;

static struct page_table *clone_ptbl(struct page_table *src, uint32_t *physical_addr)
{
	int i;
	struct page_table *table;
	
	/* Make a new page table, which is page aligned */
	table = (struct page_table *)kmalloc_ap(sizeof(struct page_table), physical_addr);
	
	/* Clear the content of the new page table */
	memset(table, 0, sizeof(struct page_table));

	DEBUG(DL_DBG, ("src (0x%x), table(0x%x)\n", src, table));

	for (i = 0; i < 1024; i++) {
		/* If the source entry has a frame associated with it */
		if (src->pages[i].frame) {

			/* Get a new frame */
			alloc_frame(&(table->pages[i]), FALSE, FALSE);

			/* Clone the flags from source to destination */
			if (src->pages[i].present) table->pages[i].present = 1;
			if (src->pages[i].rw) table->pages[i].rw = 1;
			if (src->pages[i].user) table->pages[i].user = 1;
			if (src->pages[i].accessed) table->pages[i].accessed = 1;
			if (src->pages[i].dirty) table->pages[i].dirty = 1;

			/* Physically copy the data accross */
			copy_page_physical(table->pages[i].frame * 0x1000, 
					   src->pages[i].frame * 0x1000);
		}
	}

	return table;
}

struct mmu_ctx *mmu_ctx_clone(struct mmu_ctx *src)
{
	int i;
	phys_addr_t phys_addr;
	struct mmu_ctx *ctx;
	uint32_t offset;

	/* Make a new page directory and retrieve its physical address */
	ctx = (struct mmu_ctx *)kmalloc_ap(sizeof(struct mmu_ctx), &phys_addr);
	
	/* Clear the page directory */
	memset(ctx, 0, sizeof(struct mmu_ctx));

	offset = (uint32_t)ctx->tables_pos - (uint32_t)ctx;

	/* Calculate the physical address of the page directory entry */
	ctx->phys_addr = phys_addr + offset;

	DEBUG(DL_DBG, ("src ctx(0x%x;0x%x), cloned ctx(0x%x;0x%x)\n",
		       src, src->phys_addr, ctx, ctx->phys_addr));

	/* For each page table, if the page table is in the kernel directory,
	 * do not make a new copy.
	 */
	for (i = 0; i < 1024; i++) {
		if (!src->tables[i])
			continue;
		if (_kernel_mmu_ctx.tables[i] == src->tables[i]) {
			/* It's in the kernel, so just use the same pointer */
			ctx->tables[i] = src->tables[i];
			ctx->tables_pos[i] = src->tables_pos[i];
		} else {
			/* Copy the page table */
			uint32_t physical;
			ctx->tables[i] = clone_ptbl(src->tables[i], &physical);
			ctx->tables_pos[i] = physical |
				X86_PTE_PRESENT | X86_PTE_WRITE | X86_PTE_USER;
		}
	}

	return ctx;
}

void mmu_lock_ctx(struct mmu_ctx *ctx)
{
	;
}

void mmu_unlock_ctx(struct mmu_ctx *ctx)
{
	;
}

struct page *mmu_ctx_get_page(struct mmu_ctx *ctx, uint32_t addr, boolean_t make)
{
	uint32_t pd_idx, pt_idx;

	/* Calculate the page table index and page directory index */
	pt_idx = (addr / PAGE_SIZE) % 1024;
	pd_idx = (addr / PAGE_SIZE) / 1024;

	if (ctx->tables[pd_idx]) {	// The page table already assigned
		return &ctx->tables[pd_idx]->pages[pt_idx];
	} else if (make) {		// Make a new page table
		uint32_t tmp;
		
		/* Allocate a new page table */
		ctx->tables[pd_idx] = (struct page_table *)
			kmalloc_ap(sizeof(struct page_table), &tmp);
		
		/* Clear the content of the page table */
		memset(ctx->tables[pd_idx], 0, sizeof(struct page_table));

		ctx->tables_pos[pd_idx] = tmp|X86_PTE_PRESENT|X86_PTE_WRITE|X86_PTE_USER;
		
		return &ctx->tables[pd_idx]->pages[pt_idx];
	} else {
		DEBUG(DL_INF, ("no page for addr(0x%08x) in mmu contex(0x%08x)\n",
			       addr, ctx));
		return NULL;
	}
}

void mmu_switch_ctx(struct mmu_ctx *ctx)
{
	x86_write_cr3(ctx->phys_addr);
}

void arch_init_mmu()
{
	phys_addr_t i;
	
	/* Initialize the kernel MMU context structure */
	memset(&_kernel_mmu_ctx, 0, sizeof(_kernel_mmu_ctx));
	mutex_init(&_kernel_mmu_ctx.lock, "mmu_ctx_lock", 0);
	_kernel_mmu_ctx.phys_addr = _kernel_mmu_ctx.tables_pos;

	DEBUG(DL_DBG, ("kernel mmu context(0x%08x), physical location(0x%08x)\n",
		       &_kernel_mmu_ctx, _kernel_mmu_ctx.phys_addr));
	
	mmu_lock_ctx(&_kernel_mmu_ctx);

	for (i = KERNEL_KMEM_BASE; i < KERNEL_KMEM_BASE + KERNEL_KMEM_SIZE; i += PAGE_SIZE)
		mmu_ctx_get_page(&_kernel_mmu_ctx, i, TRUE);

	i = 0;
	/* Do identity map for the memory we used */
	while (i < (_placement_addr + PAGE_SIZE)) {
		/* Kernel code is readable but not writable from user-mode */
		alloc_frame(mmu_ctx_get_page(&_kernel_mmu_ctx, i, TRUE), FALSE, FALSE);
		i += PAGE_SIZE;
	}

	for (i = KERNEL_KMEM_BASE; i < KERNEL_KMEM_BASE + KERNEL_KMEM_SIZE; i += PAGE_SIZE)
		alloc_frame(mmu_ctx_get_page(&_kernel_mmu_ctx, i, TRUE), FALSE, FALSE);
	
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
