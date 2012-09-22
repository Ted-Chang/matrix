/*
 * mmu.c
 */

#include <types.h>
#include <string.h>	// memset
#include "hal.h"
#include "isr.h"	// register interrupt handler
#include "mm/mmu.h"
#include "mm/kmem.h"
#include "matrix/debug.h"

/* Determine if an MMU context is the kernel context */
#define IS_KERNEL_CTX(ctx)	(ctx == &_kernel_mmu_ctx)

/* Determine if an MMU context is the current context */
#define IS_CURRENT_CTX(ctx)	(ctx == _current_mmu_ctx)

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
	/* Page directory entry for the page table below, this must be the
	 * first member of pdir structure because we use the physical address
	 * of this structure as the PDBR.
	 */
	uint32_t pde[1024];

	/* Page tables for this page directory */
	struct ptbl *ptbl[1024];
};

struct mmu_ctx _kernel_mmu_ctx;
struct mmu_ctx *_current_mmu_ctx = NULL;
static struct irq_hook _pf_hook;

extern uint32_t _placement_addr;

extern struct heap *_kheap;

extern void copy_page_physical(uint32_t dst, uint32_t src);

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

			/* Get a new frame */
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

struct page *mmu_get_page(struct mmu_ctx *ctx, uint32_t virt, boolean_t make,
			  int mmflag)
{
	uint32_t dir_idx, tbl_idx;
	struct pdir *pdir;

	pdir = ctx->pdir;

	/* Calculate the page table index and page directory index */
	tbl_idx = (virt / PAGE_SIZE) % 1024;
	dir_idx = (virt / PAGE_SIZE) / 1024;

	if (pdir->ptbl[dir_idx]) {	// The page table already assigned
		return &pdir->ptbl[dir_idx]->pte[tbl_idx];
	} else if (make) {		// Make a new page table
		uint32_t tmp;
		
		/* Allocate a new page table */
		pdir->ptbl[dir_idx] = (struct ptbl *)kmem_alloc_ap(sizeof(struct ptbl), &tmp);
		
		/* Clear the content of the page table */
		memset(pdir->ptbl[dir_idx], 0, sizeof(struct ptbl));

		/* Set the content of the page table */
		pdir->pde[dir_idx] = tmp | 0x7;	// PRESENT, RW, US.
		
		return &pdir->ptbl[dir_idx]->pte[tbl_idx];
	} else {
		DEBUG(DL_INF, ("no page for addr(0x%08x) in mmu context(0x%08x)\n",
			       virt, ctx));
		return NULL;
	}
}

int mmu_map_page(struct mmu_ctx *ctx, uint32_t virt, phys_addr_t phys,
		 boolean_t write, boolean_t execute, int mmflag)
{
	struct page *page;

	/* Find the page table for the virtual address */
	page = mmu_get_page(ctx, virt, TRUE, 0);
	if (!page) {
		return -1;
	}

	/* Check if the mapping already exists */
	if (page->present)
		PANIC("Virtual address already mapped.");

	/* Set the PTE */
	page->present = 1;

	if (write)
		page->rw = 1;

	if (!IS_KERNEL_CTX(ctx))
		page->user = 1;

	page->frame = phys >> 12;

	return 0;
}

int mmu_unmap_page(struct mmu_ctx *ctx, uint32_t virt, boolean_t shared,
		   phys_addr_t *phys)
{
	uint32_t pte, pde;
	struct ptbl *ptbl;
	uint32_t paddr;

	/* Find the page table for the virtual address */
	pde = (virt / PAGE_SIZE) / 1024;
	ptbl = ctx->pdir->ptbl[pde];

	/* If the mapping doesn't exist we did nothing */
	pte = (virt / PAGE_SIZE) % 1024;
	if (!ptbl->pte[pte].present)
		return -1;

	/* Save the physical address */
	paddr = ptbl->pte[pte].frame << 12;

	/* Clear the entry and invalidate the TLB entry */
	memset(&ptbl->pte[pte], 0, sizeof(struct page));

	if (phys)
		*phys = paddr;

	return 0;
}

void mmu_switch_ctx(struct mmu_ctx *ctx)
{
	uint32_t cr0;

	ASSERT((ctx->pdbr % 4096) == 0);

	/* Update the current mmu context */
	_current_mmu_ctx = ctx;
	
	DEBUG(DL_DBG, ("switch context to 0x%08x, pdbr(0x%08x)\n", ctx, ctx->pdbr));

	/* Set CR3 register */
	asm volatile("mov %0, %%cr3":: "r"(ctx->pdbr));
	
	asm volatile("mov %%cr0, %0": "=r"(cr0));
	cr0 |= 0x80000000;	// Enable paging
	asm volatile("mov %0, %%cr0":: "r"(cr0));
}

/*
 * We don't call irq_done here, check this when we implementing
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

	/* Print an error message */
	kprintf("Page fault(%s%s%s%s) at 0x%x - EIP: 0x%x\n", 
		present ? "present " : "non-present ",
		rw ? "write " : "read ",
		us ? "user-mode " : "supervisor-mode ",
		reserved ? "reserved " : "",
		faulting_addr,
		regs->eip);

	PANIC("Page fault");
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
			dst_dir->pde[i] = pde | 0x07;
		}
	}
}

struct mmu_ctx *mmu_create_ctx()
{
	struct mmu_ctx *ctx;
	uint32_t pdbr;

	ctx = kmem_alloc(sizeof(struct mmu_ctx));
	if (!ctx)
		return NULL;

	ctx->pdir = kmem_alloc_ap(sizeof(struct pdir), &pdbr);
	if (!ctx->pdir) {
		kmem_free(ctx);
		return NULL;
	}
	ctx->pdbr = pdbr;
	memset(ctx->pdir, 0, sizeof(struct pdir));

	return ctx;
}

void mmu_destroy_ctx(struct mmu_ctx *ctx)
{
	ASSERT(!IS_KERNEL_CTX(ctx));

	kmem_free(ctx->pdir);
	kmem_free(ctx);
}

void init_mmu()
{
	phys_addr_t i, pdbr;
	struct page *page;

	/* Initialize the kernel MMU context structure */
	_kernel_mmu_ctx.pdir = kmem_alloc_ap(sizeof(struct pdir), &pdbr);
	_kernel_mmu_ctx.pdbr = pdbr;
	memset(_kernel_mmu_ctx.pdir, 0, sizeof(struct pdir));
	
	DEBUG(DL_DBG, ("kernel mmu context(0x%08x), page directory address(0x%08x)\n",
		       &_kernel_mmu_ctx, _kernel_mmu_ctx.pdbr));

	/* Allocate some pages in the kernel heap area. Here we call mmu_get_page
	 * but not page_alloc. this cause the page tables to be created when necessary.
	 * We can't allocate page yet because they need to be identity mapped first.
	 */
	for (i = KHEAP_START; i < KHEAP_START+KHEAP_INITIAL_SIZE; i += PAGE_SIZE)
		mmu_get_page(&_kernel_mmu_ctx, i, TRUE, 0);

	/* Do identity map (physical addr == virtual addr) for the memory we used. */
	for (i = 0; i < (_placement_addr+PAGE_SIZE); i += PAGE_SIZE) {
		/* Kernel code is readable but not writable from user-mode */
		page = mmu_get_page(&_kernel_mmu_ctx, i, TRUE, 0);
		page_alloc(page, FALSE, FALSE);
	}

	/* Allocate those pages we mapped earlier, our kernel heap start from
	 * address 0xC0000000 and size is 0x100000
	 */
	for (i = KHEAP_START; i < KHEAP_START+KHEAP_INITIAL_SIZE; i += PAGE_SIZE) {
		page = mmu_get_page(&_kernel_mmu_ctx, i, TRUE, 0);
		page_alloc(page, FALSE, FALSE);
	}

	/* Before we enable paging, we must register our page fault handler */
	register_irq_handler(14, &_pf_hook, page_fault);

	/* Enable paging now */
	mmu_switch_ctx(&_kernel_mmu_ctx);
}
