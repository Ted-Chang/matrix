/*
 * mmu.c
 */

#include <types.h>
#include <string.h>	// memset
#include <errno.h>
#include "hal/hal.h"
#include "hal/core.h"
#include "hal/isr.h"
#include "mm/mm.h"
#include "mm/mlayout.h"
#include "mm/mmu.h"
#include "mm/kmem.h"
#include "mm/malloc.h"
#include "debug.h"
#include "proc/process.h"
#include "proc/thread.h"

/*
 * Page Table
 * Each page table has 1024 page table entries, actually each
 * page table entry is just a 32 bit integer for X86 arch
 */
struct ptbl {
	struct page pte[1024];
};

/*
 * Page Directory
 * Each page directory has 1024 pointers to its page table entry
 */
struct pdir {
	/* Page directory entry for the page table below, this must be
	 * page aligned.
	 */
	uint32_t pde[1024];

	/* Page tables for this page directory */
	struct ptbl *ptbl[1024];
};

struct mmu_ctx _kernel_mmu_ctx;

extern uint32_t _placement_addr;
extern isr_t _isr_table[];

static void *alloc_structure(size_t size, phys_addr_t *phys, int mmflag)
{
	void *ret;

	/* Try to allocate from kernel memory pool first, if failed then do
	 * page early alloc
	 */
	if (_kmem_init_done) {
		ret = kmem_alloc(size, mmflag);
		if (ret) {
			struct page *page;
			page = mmu_get_page(&_kernel_mmu_ctx, (uint32_t)ret, FALSE, 0);
			ASSERT(page != NULL);
			(*phys) = page->frame * PAGE_SIZE + ((uint32_t)ret & 0xFFF);
		}
	} else {
		page_early_alloc(phys, size, IS_FLAG_ON(mmflag, MM_ALIGN));
		ret = (void *)(*phys);
		ASSERT(ret != NULL);
	}
	
	if (FLAG_ON(mmflag, MM_ALIGN)) {
		ASSERT((((ptr_t)ret) % PAGE_SIZE) == 0);
	}
	
	return ret;
}

static struct ptbl *clone_ptbl(struct ptbl *src, phys_addr_t *phys_addr)
{
	int i;
	struct ptbl *ptbl;
	
	/* Make a new page table, which is page aligned */
	ptbl = (struct ptbl *)alloc_structure(sizeof(struct ptbl), phys_addr,
					      MM_ALIGN);
	
	/* Clear the content of the new page table */
	memset(ptbl, 0, sizeof(struct ptbl));

	/* Clone each of the page frames */
	for (i = 0; i < 1024; i++) {
		/* If the source entry has a frame associated with it */
		if (src->pte[i].frame) {

			/* Get a new frame */
			page_alloc(&(ptbl->pte[i]), 0);

			/* Clone the flags from source to destination */
			if (src->pte[i].present) ptbl->pte[i].present = 1;
			if (src->pte[i].rw) ptbl->pte[i].rw = 1;
			if (src->pte[i].user) ptbl->pte[i].user = 1;
			if (src->pte[i].accessed) ptbl->pte[i].accessed = 1;
			if (src->pte[i].dirty) ptbl->pte[i].dirty = 1;

			/* Physically copy the data accross */
			page_copy(ptbl->pte[i].frame * 0x1000, 
				  src->pte[i].frame * 0x1000);
		}
	}

	return ptbl;
}

/**
 * Get a page from the specified mmu context
 * @ctx		- mmu context
 * @virt	- virtual address to map the memory to
 * @make	- make a new page table if we are out of page table
 * @mmflag	- memory manager flags
 */
struct page *mmu_get_page(struct mmu_ctx *ctx, ptr_t virt, boolean_t make, int mmflag)
{
	struct page *page;
	uint32_t dir_idx, tbl_idx;
	struct pdir *pdir;

	ASSERT(ctx != NULL);

	/* Calculate the page table index and page directory index */
	tbl_idx = (virt / PAGE_SIZE) % 1024;
	dir_idx = (virt / PAGE_SIZE) / 1024;

	/* Get the page directory from the context */
	pdir = ctx->pdir;

	if (pdir->ptbl[dir_idx]) {	// The page table already assigned
		page = &pdir->ptbl[dir_idx]->pte[tbl_idx];
	} else if (make) {		// Make a new page table
		phys_addr_t tmp;
		
		/* Allocate a new page table */
		pdir->ptbl[dir_idx] = (struct ptbl *)
			alloc_structure(sizeof(struct ptbl), &tmp, MM_ALIGN);
		
		/* Clear the content of the page table */
		memset(pdir->ptbl[dir_idx], 0, sizeof(struct ptbl));

		/* Set the content of the page table */
		pdir->pde[dir_idx] = tmp | 0x7;	// PRESENT, RW, US.
		
		page = &pdir->ptbl[dir_idx]->pte[tbl_idx];
	} else {
		DEBUG(DL_INF, ("no page for addr(0x%08x) in mmu ctx(0x%08x)\n",
			       virt, ctx));
		page = NULL;
	}

	return page;
}

int mmu_map(struct mmu_ctx *ctx, ptr_t virt, phys_addr_t phys, int flags)
{
	int rc;
	struct page *p;

	ASSERT((phys % PAGE_SIZE) == 0);
	
	/* Get a page from the specified MMU context */
	p = mmu_get_page(ctx, virt, TRUE, flags);
	if (!p) {
		DEBUG(DL_WRN, ("get page failed, ctx(%p) virt(%x)\n", ctx, virt));
		rc = EINVAL;
		goto out;
	}

	/* The page should not be present */
	if (p->present) {
		DEBUG(DL_WRN, ("Mapping already mapped address(%x) ctx(%p)\n",
			       virt, ctx));
		rc = EMAPPED;
		goto out;
	}

	/* Set the page table entry */
	p->frame = phys / PAGE_SIZE;
	p->present = 1;
	p->user = IS_KERNEL_CTX(ctx) ? FALSE : TRUE;
	p->rw = FLAG_ON(flags, MMU_MAP_WRITE) ? TRUE : FALSE;
	
	rc = 0;

 out:
	return rc;
}

int mmu_unmap(struct mmu_ctx *ctx, ptr_t virt, boolean_t shared, phys_addr_t *physp)
{
	int rc;
	struct page *p;
	phys_addr_t entry;

	/* Get the page which the specified virtual address was mapping */
	p = mmu_get_page(ctx, virt, FALSE, 0);
	if (!p) {
		DEBUG(DL_WRN, ("get page failed, ctx(%p) virt(%x)\n", ctx, virt));
		rc = EINVAL;
		goto out;
	}

	/* The mapping does not exist, just return */
	if (!p->present) {
		DEBUG(DL_WRN, ("unmap non-present page, ctx(%p) virt(%x)\n",
			       ctx, virt));
		rc = -1;
		goto out;
	}

	entry = p->frame;
	p->frame = 0;
	p->present = 0;

	if (physp) {
		*physp = entry;
	}

	rc = 0;

 out:
	return rc;
}

void mmu_load_ctx(struct mmu_ctx *ctx)
{
	ASSERT((ctx->pdbr % PAGE_SIZE) == 0);

	/* Set CR3 register */
	x86_write_cr3(ctx->pdbr);
}

/*
 * We don't call local_irq_done here, check this when we implementing
 * the paging feature of our kernel.
 */
void page_fault(struct registers *regs)
{
	uint32_t faulting_addr;
	int present;
	int rw;
	int us;
	int reserved;

	/* A page fault has occurred. The CR2 register
	 * contains the faulting address.
	 */
	asm volatile("mov %%cr2, %0" : "=r"(faulting_addr));

	present = regs->err_code & 0x1;
	rw = regs->err_code & 0x2;
	us = regs->err_code & 0x4;
	reserved = regs->err_code & 0x8;

	dump_registers(regs);

	/* Print an error message */
	kprintf("Page fault(%s%s%s%s) at 0x%x - EIP: 0x%x\n\n", 
		present ? "present " : "non-present ",
		rw ? "write " : "read ",
		us ? "user-mode " : "supervisor-mode ",
		reserved ? "reserved " : "",
		faulting_addr,
		regs->eip);

	PANIC("Page fault");
}

void mmu_clone_ctx(struct mmu_ctx *dst, struct mmu_ctx *src)
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
		if (!src_dir->ptbl[i]) {
			/* Already allocated */
			continue;
		}
		
		if (krn_dir->ptbl[i] == src_dir->ptbl[i]) {
			/* It's in the kernel, so just use the same page table
			 * and page directory entry
			 */
			dst_dir->ptbl[i] = src_dir->ptbl[i];
			dst_dir->pde[i] = src_dir->pde[i];
		} else {
			/* Physically clone the page table if it's not kernel stuff */
			uint32_t pde;

			DEBUG(DL_DBG, ("dst(0x%x), src(0x%x), addr(0x%x).\n",
				       dst, src, i * 1024 * PAGE_SIZE));
			dst_dir->ptbl[i] = clone_ptbl(src_dir->ptbl[i], &pde);
			dst_dir->pde[i] = pde | 0x07;
		}
	}
}

struct mmu_ctx *mmu_create_ctx()
{
	struct mmu_ctx *ctx;
	phys_addr_t pdbr;

	ctx = kmem_alloc(sizeof(struct mmu_ctx), 0);
	if (!ctx) {
		goto out;
	}

	ctx->pdir = alloc_structure(sizeof(struct pdir), &pdbr, MM_ALIGN);
	if (!ctx->pdir) {
		kmem_free(ctx);
		goto out;
	}
	memset(ctx->pdir, 0, sizeof(struct pdir));
	ctx->pdbr = pdbr;
	ASSERT((ctx->pdbr % PAGE_SIZE) == 0);

	mutex_init(&ctx->lock, "mmu-mutex", 0);	// TODO: flags need to be confirmed

 out:
	return ctx;
}

void mmu_destroy_ctx(struct mmu_ctx *ctx)
{
	ASSERT(!IS_KERNEL_CTX(ctx));

	kmem_free(ctx->pdir);
	kmem_free(ctx);
}

void init_mmu_percore()
{
	/* Load kernel mmu context into this core */
	mmu_load_ctx(&_kernel_mmu_ctx);
	
	/* Enable paging */
	x86_write_cr0(x86_read_cr0() | X86_CR0_PG);
}

void init_mmu()
{
	phys_addr_t i;
	phys_addr_t pdbr;
	struct page *page;

	/* Initialize the kernel MMU context structure */
	_kernel_mmu_ctx.pdir = alloc_structure(sizeof(struct pdir), &pdbr, MM_ALIGN);
	_kernel_mmu_ctx.pdbr = pdbr;
	memset(_kernel_mmu_ctx.pdir, 0, sizeof(struct pdir));
	
	DEBUG(DL_DBG, ("kernel MMU context(%p), pdbr(%p), core(%p)\n",
		       &_kernel_mmu_ctx, _kernel_mmu_ctx.pdbr, CURR_CORE));

	/* Allocate some pages in the kernel pool area. Here we call mmu_get_page
	 * but we do not call page_alloc. this cause the page tables to be created
	 * when necessary. We cannot allocate pages yet because they need to be
	 * identity mapped first.
	 */
	for (i = KERNEL_KMEM_START;
	     i < (KERNEL_KMEM_START + KERNEL_KMEM_SIZE);
	     i += PAGE_SIZE) {
		mmu_get_page(&_kernel_mmu_ctx, i, TRUE, 0);
	}

	/* Do identity map (physical addr == virtual addr) for the memory we
	 * have used.
	 */
	for (i = 0; i < (_placement_addr + PAGE_SIZE); i += PAGE_SIZE) {
		/* Kernel code is readable but not writable from user-mode */
		page = mmu_get_page(&_kernel_mmu_ctx, i, TRUE, 0);
		page_alloc(page, 0);
		page->user = FALSE;
		page->rw = FALSE;
	}

	/* Allocate those pages we mapped for kernel pool area */
	for (i = KERNEL_KMEM_START;
	     i < (KERNEL_KMEM_START + KERNEL_KMEM_SIZE);
	     i += PAGE_SIZE) {
		page = mmu_get_page(&_kernel_mmu_ctx, i, FALSE, 0);
		ASSERT(page != NULL);
		page_alloc(page, 0);
		page->user = FALSE;
		page->rw = FALSE;
	}

	/* Before we enable paging, we must register our page fault handler */
	_isr_table[X86_TRAP_PF] = page_fault;

	/* Switch to our kernel mmu context, kernel mmu context will be used
	 * by every process.
	 */
	mmu_load_ctx(&_kernel_mmu_ctx);

	/* Enable paging */
	x86_write_cr0(x86_read_cr0() | X86_CR0_PG);
}
