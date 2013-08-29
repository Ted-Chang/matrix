#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "debug.h"
#include "mm/mm.h"
#include "mm/mlayout.h"
#include "mm/page.h"
#include "mm/mmu.h"

#define PMAP_CONTAINS(addr, size)					\
	((addr >= KERNEL_PMAP_START) &&					\
	 ((addr + size) <= (KERNEL_PMAP_START + KERNEL_PMAP_SIZE)))

void *phys_map(phys_addr_t addr, size_t size, int mmflag)
{
	ptr_t ptr;
	phys_addr_t base;
	void *ret = NULL;

	if (!size) {
		/* What do you want to do??? */
		ASSERT(0);
		goto out;
	}

	/* Use the physical map area if the range lies within it. For more
	 * information please refer to the memory layout of our system.
	 */
	if (PMAP_CONTAINS(addr, size)) {
		ret = (void *)addr;
		goto out;
	}

	DEBUG(DL_DBG, ("addr(%x), size(%x).\n", addr, size));

	ASSERT(0);
 out:

	return ret;
}

void phys_unmap(void *addr, size_t size, boolean_t shared)
{
	/* If the memory range lies within the physical map area, we don't
	 * need to do anything. Otherwise, unmap and free the kernel memory.
	 */
	if (((uint32_t)addr < KERNEL_PMAP_START) ||
	    ((uint32_t)addr > (KERNEL_PMAP_START + KERNEL_PMAP_SIZE))) {
		// TODO: Implement this.
	}
}
