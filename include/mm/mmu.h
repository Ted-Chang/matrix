#ifndef __MMU_H__
#define __MMU_H__

/* Definitions of page attributes */
#define X86_PTE_PRESENT		(1<<0)	// Page is present
#define X86_PTE_WRITE		(1<<1)	// Page is writable
#define X86_PTE_USER		(1<<2)	// Page is accessible in CPL3
#define X86_PTE_PWT		(1<<3)	// Page has write-through caching

#include "mutex.h"

struct page_table {
	struct page pages[1024];
};

struct mmu_ctx {
	struct mutex lock;

	struct page_table *tables[1024];

	uint32_t tables_pos[1024];
	
	uint32_t phys_addr;
};

/* Kernel MMU context */
extern struct mmu_ctx _kernel_mmu_ctx;

extern struct page *mmu_ctx_get_page(struct mmu_ctx *ctx, uint32_t addr, boolean_t make);
extern void mmu_switch_ctx();

extern void arch_init_mmu();
extern void arch_init_mmu_per_cpu();

extern void init_mmu();
extern void init_mmu_per_cpu();

#endif	/* __MMU_H__ */
