#include <types.h>
#include <stddef.h>
#include "list.h"
#include "mm/page.h"
#include "mm/mmu.h"

struct mmu_ctx _kernel_mmu_ctx;

void mmu_switch_ctx()
{
	;
}

void arch_init_mmu()
{
	;
}

void arch_init_mmu_per_cpu()
{
	;
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
