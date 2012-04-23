#ifndef __MMU_H__
#define __MMU_H__

/* Definitions of paging structure bits */
#define X86_PTE_PRESENT		(1<<0)
#define X86_PTE_WRITE		(1<<1)
#define X86_PTE_USER		(1<<2)
#define X86_PTE_PWT		(1<<3)	// Page has write-through caching
#define X86_PTE_PCD		(1<<4)	// Page has caching disabled
#define X86_PTE_ACCESSED	(1<<5)	// Page has been accessed
#define X86_PTE_DIRTY		(1<<6)	// Page has been written to
#define X86_PTE_LARGE		(1<<7)	// Page is a large page
#define X86_PTE_GLOBAL		(1<<8)	// Page won't be cleared in TLB
#define X86_PTE_NOEXEC		(1<<63)	// Page is not executable (requires NX support)

#include "mutex.h"

/* Size of TLB flush array */
#define INVALIDATE_ARRAY_SIZE	128

struct mmu_ctx {
	struct mutex lock;	// Lock to protect context
	phys_addr_t pml4;	// Physical address of the PML4
	
	/* Array of TLB entries to flush when unlocking context. Note
	 * that if the count becomes greater than the array size, then
	 * the entire TLB will be flushed.
	 */
	uint32_t pages_to_invalidate[INVALIDATE_ARRAY_SIZE];
	size_t invalidate_count;
};

/* Boot strap page */
extern phys_addr_t _ap_bootstrap_page;

/* Kernel MMU context */
extern struct mmu_ctx _kernel_mmu_ctx;

extern struct mmu_ctx *mmu_create_ctx(int mmflag);
extern void mmu_destroy_ctx(struct mmu_ctx *ctx);
extern void mmu_switch_ctx();

extern void arch_init_mmu();
extern void arch_init_mmu_per_cpu();

extern void init_mmu();
extern void init_mmu_per_cpu();

#endif	/* __MMU_H__ */
