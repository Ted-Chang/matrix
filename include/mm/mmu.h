#ifndef __MMU_H__
#define __MMU_H__

/* Size of TLB flush array */
#define INVALIDATE_ARRAY_SIZE	128

struct mmu_ctx {
	uint64_t pml4;		// Physical address of the PML4
	
	/* Array of TLB entries to flush when unlocking context. Note
	 * that if the count becomes greater than the array size, then
	 * the entire TLB will be flushed.
	 */
	uint32_t pages_to_invalidate[INVALIDATE_ARRAY_SIZE];
	size_t invalidate_count;
};

/* Boot strap page */
uint32_t _ap_bootstrap_page;

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
