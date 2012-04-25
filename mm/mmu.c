#include <types.h>
#include <stddef.h>
#include <string.h>
#include "list.h"
#include "cpu.h"
#include "mm/mlayout.h"
#include "mm/mm.h"
#include "mm/page.h"
#include "mm/mmu.h"
#include "mm/kmem.h"
#include "mm/vaspace.h"
#include "matrix/debug.h"
#include "status.h"

/* Check if an MMU context is the kernel context */
#define IS_KERNEL_CTX(ctx)	(ctx == &_kernel_mmu_ctx)

/* Determine if an MMU context is the current context */
#define IS_CURRENT_CTX(ctx)	(IS_KERNEL_CTX(ctx) || \
				 (CURR_SPACE && (ctx == CURR_SPACE->mmu)))

/* Definitions of a page table */
struct ptbl {
	struct page pte[1024];
};

/* Definitions of a page directory */
struct pdir {
	/* Page directory entry for the page tables below, this must be the
	 * first member of pdir structure because we use the physical address
	 * of this structure as the PDBR.
	 */
	uint32_t pde[1024];
	
	/* Page tables for this page directory */
	struct ptbl *ptbl[1024];
};

extern void copy_page_physical(uint32_t dst, uint32_t src);

/* Kernel MMU context */
struct mmu_ctx _kernel_mmu_ctx;

static struct ptbl *clone_ptbl(struct ptbl *src, uint32_t *phys_addr)
{
	int i;
	struct ptbl *ptbl;
	
	/* Make a new page table, which is page aligned */
	ptbl = (struct ptbl *)kmem_alloc_ap(sizeof(struct ptbl), phys_addr);
	
	/* Clear the content of the new page table */
	memset(ptbl, 0, sizeof(struct ptbl));

	DEBUG(DL_DBG, ("src page table(0x%x), new page table(0x%x)\n", src, ptbl));

	for (i = 0; i < 1024; i++) {
		/* If the source entry has a frame associated with it */
		if (src->pte[i].frame) {

			/* Allocate a new frame */
			page_alloc(&(ptbl->pte[i]), FALSE, FALSE);

			/* Clone the flags from source to destination */
			if (src->pte[i].present) ptbl->pte[i].present = 1;
			if (src->pte[i].rw) ptbl->pte[i].rw = 1;
			if (src->pte[i].user) ptbl->pte[i].user = 1;
			if (src->pte[i].accessed) ptbl->pte[i].accessed = 1;
			if (src->pte[i].dirty) ptbl->pte[i].dirty = 1;

			/* Physically copy the data accross */
			copy_page_physical(ptbl->pte[i].frame * 0x1000, 
					   src->pte[i].frame * 0x1000);
		}
	}

	return ptbl;
}

void mmu_copy_ctx(struct mmu_ctx *dst, struct mmu_ctx *src)
{
	int i;
	struct pdir *dst_dir, *src_dir, *krn_dir;

	dst_dir = dst->pdir;
	src_dir = src->pdir;
	krn_dir = _kernel_mmu_ctx.pdir;
	
	/* For each page table, if the page table is in the kernel directory,
	 * do not make a new copy.
	 */
	for (i = 0; i < 1024; i++) {
		if (!src_dir->ptbl[i])
			continue;
		
		if (krn_dir->ptbl[i] == src_dir->ptbl[i]) {
			/* It's in the kernel, so just use the same page table
			 * and page directory entry
			 */
			dst_dir->ptbl[i] = src_dir->ptbl[i];
			dst_dir->pde[i] = src_dir->pde[i];
		} else {
			/* Physically copy the page table */
			uint32_t pde;
			dst_dir->ptbl[i] = clone_ptbl(src_dir->ptbl[i], &pde);
			dst_dir->pde[i] =
				pde|X86_PTE_PRESENT|X86_PTE_WRITE|X86_PTE_USER;
		}
	}
}

static void mmu_invalidate_ctx(struct mmu_ctx *ctx, uint32_t virt, boolean_t shared)
{
	/* Invalidate on the current CPU if we're using this context */
	if (IS_CURRENT_CTX(ctx))
		x86_invlpg(virt);
}

void mmu_lock_ctx(struct mmu_ctx *ctx)
{
	mutex_acquire(&ctx->lock);
}

void mmu_unlock_ctx(struct mmu_ctx *ctx)
{
	mutex_release(&ctx->lock);
}

struct page *mmu_get_pages(struct mmu_ctx *ctx, uint32_t addr, boolean_t make)
{
	uint32_t pd_idx, pt_idx;
	struct pdir *pdir;

	pdir = ctx->pdir;
	
	/* Calculate the page table index and page directory index */
	pt_idx = (addr / PAGE_SIZE) % 1024;
	pd_idx = (addr / PAGE_SIZE) / 1024;

	if (pdir->ptbl[pd_idx]) {	// The page table already assigned
		return &pdir->ptbl[pd_idx]->pte[pt_idx];
	} else if (make) {		// Make a new page table
		uint32_t tmp;
		
		/* Allocate a new page table */
		pdir->ptbl[pd_idx] = (struct ptbl *)kmem_alloc_ap(sizeof(struct ptbl), &tmp);
		
		/* Clear the content of the page table */
		memset(pdir->ptbl[pd_idx], 0, sizeof(struct ptbl));

		pdir->pde[pd_idx] = tmp|X86_PTE_PRESENT|X86_PTE_WRITE|X86_PTE_USER;
		
		return &pdir->ptbl[pd_idx]->pte[pt_idx];
	} else {
		DEBUG(DL_INF, ("no page for addr(0x%08x) in mmu contex(0x%08x)\n",
			       addr, ctx));
		return NULL;
	}
}

void mmu_switch_ctx(struct mmu_ctx *ctx)
{
	uint32_t cr0;
	
	ASSERT((ctx->pdbr % 4096) == 0);

	//x86_write_cr3(ctx->pdbr);
	//x86_write_cr0(x86_read_cr0() | 0x80000000);

	kprintf("switch context to 0x%08x, pdbr(0x%08x)\n", ctx, ctx->pdbr);

	asm volatile("mov %0, %%cr3":: "r"(ctx->pdbr));
	asm volatile("mov %%cr0, %0": "=r"(cr0));
	cr0 |= 0x80000000;
	asm volatile("mov %0, %%cr0":: "r"(cr0));
}

struct mmu_ctx *mmu_create_ctx()
{
	struct mmu_ctx *ctx;
	phys_addr_t pdbr;

	ctx = kmem_alloc(sizeof(struct mmu_ctx));
	if (!ctx)
		return NULL;

	mutex_init(&ctx->lock, "mmu_ctx_lock", 0);
	ctx->pdir = kmem_alloc_ap(sizeof(struct pdir), &pdbr);
	if (!ctx->pdir) {
		kmem_free(ctx);
		return NULL;
	}
	ctx->pdbr = pdbr;
	memset(ctx->pdir, 0, sizeof(struct pdir));

	/* Get the kernel mappings into the new page directory */
	mmu_copy_ctx(ctx, &_kernel_mmu_ctx);

	return ctx;
}

void mmu_destroy_ctx(struct mmu_ctx *ctx)
{
	ASSERT(!IS_KERNEL_CTX(ctx));

	kmem_free(ctx->pdir);
	kmem_free(ctx);
}

void arch_init_mmu()
{
	phys_addr_t i, pdbr;
	
	/* Initialize the kernel MMU context structure */
	mutex_init(&_kernel_mmu_ctx.lock, "mmu_ctx_lock", 0);
	_kernel_mmu_ctx.pdir = kmem_alloc_ap(sizeof(struct pdir), &pdbr);
	_kernel_mmu_ctx.pdbr = pdbr;
	memset(_kernel_mmu_ctx.pdir, 0, sizeof(struct pdir));

	DEBUG(DL_DBG, ("kernel mmu context(0x%08x), page directory address(0x%08x)\n",
		       &_kernel_mmu_ctx, _kernel_mmu_ctx.pdbr));
	
	mmu_lock_ctx(&_kernel_mmu_ctx);

	/* Map some pages in the kernel heap area. Here we call mmu_ctx_get_page
	 * but not page_alloc, this cause the page tables to be created when necessary.
	 * We can't allocate page yet because they need to be identity mapped first.
	 */
	for (i = KERNEL_PMAP_BASE;
	     i < KERNEL_PMAP_BASE + KERNEL_PMAP_SIZE;
	     i += PAGE_SIZE)
		mmu_get_pages(&_kernel_mmu_ctx, i, TRUE);

	i = 0;
	/* Do identity map for the memory we used */
	while (i < (_placement_addr + PAGE_SIZE)) {
		/* Kernel code is readable but not writable from user-mode */
		page_alloc(mmu_get_pages(&_kernel_mmu_ctx, i, TRUE), FALSE, FALSE);
		i += PAGE_SIZE;
	}

	/* Allocate those pages we mapped earlier */
	for (i = KERNEL_PMAP_BASE;
	     i < KERNEL_PMAP_BASE + KERNEL_PMAP_SIZE;
	     i += PAGE_SIZE)
		page_alloc(mmu_get_pages(&_kernel_mmu_ctx, i, TRUE), FALSE, FALSE);
	
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
