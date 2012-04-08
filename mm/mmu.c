#include <types.h>
#include "mm/mmu.h"

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
