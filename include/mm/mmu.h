#ifndef __MMU_H__
#define __MMU_H__

/* Definitions of page attributes */
#define X86_PTE_PRESENT		(1<<0)	// Page is present
#define X86_PTE_WRITE		(1<<1)	// Page is writable
#define X86_PTE_USER		(1<<2)	// Page is accessible in CPL3
#define X86_PTE_PWT		(1<<3)	// Page has write-through caching
#define X86_PTE_PCD		(1<<4)	// Page has caching disabled
#define X86_PTE_ACCESSED	(1<<5)	// Page has been accessed
#define X86_PTE_DIRTY		(1<<6)	// Page has been written to
#define X86_PTE_LARGE		(1<<7)	// Page is a large page
#define X86_PTE_GLOBAL		(1<<8)	// Page won't be cleared in TLB

#include "status.h"
#include "mutex.h"

struct pdir;

struct mmu_ctx {
	struct mutex lock;		// Lock to protect the context

	struct pdir *pdir;		// Virtual address of the page directory
	uint32_t pdbr;			// Physical address of the page directory
};

/* Kernel MMU context */
extern struct mmu_ctx _kernel_mmu_ctx;

extern struct page *mmu_get_page(struct mmu_ctx *ctx, uint32_t addr,
				 boolean_t make, int mmflag);
extern status_t mmu_map_page(struct mmu_ctx *ctx, uint32_t virt, phys_addr_t phys,
			     boolean_t write, boolean_t execute, int mmflag);
extern boolean_t mmu_unmap_page(struct mmu_ctx *ctx, uint32_t virt, boolean_t shared,
				phys_addr_t *phys);

extern void mmu_lock_ctx(struct mmu_ctx *ctx);
extern void mmu_unlock_ctx(struct mmu_ctx *ctx);
extern void mmu_switch_ctx();
extern struct mmu_ctx *mmu_create_ctx();
extern void mmu_destroy_ctx(struct mmu_ctx *ctx);

extern void arch_init_mmu();
extern void arch_init_mmu_per_cpu();

extern void init_mmu();
extern void init_mmu_per_cpu();

#endif	/* __MMU_H__ */
