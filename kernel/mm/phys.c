#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "debug.h"
#include "mm/mm.h"
#include "mm/mlayout.h"
#include "mm/page.h"
#include "mm/kmem.h"

#define PMAP_CONTAINS(addr, size)					\
	((addr >= KERNEL_PMAP_START) &&					\
	 ((addr + size) <= (KERNEL_PMAP_START + KERNEL_PMAP_SIZE)))

void *phys_map(phys_addr_t addr, size_t size, int mmflag)
{
	phys_addr_t base;
	phys_addr_t end;
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

	base = ROUND_DOWN(addr, PAGE_SIZE);
	end = ROUND_UP(addr + size, PAGE_SIZE);
	ASSERT(end > base);
	
	/* Map pages from kernel memory */
	ret = kmem_map(base, end - base, mmflag);
	
	DEBUG(DL_DBG, ("addr(%x), size(%x), ret(%p).\n", addr, size, ret));

 out:
	return ret;
}

void phys_unmap(void *addr, size_t size, boolean_t shared)
{
	ptr_t base;
	ptr_t end;

	/* If the memory range lies within the physical map area, we don't
	 * need to do anything. Otherwise, unmap and free the kernel memory.
	 */
	if (((uint32_t)addr < KERNEL_PMAP_START) ||
	    ((uint32_t)addr > (KERNEL_PMAP_START + KERNEL_PMAP_SIZE))) {
		base = ROUND_DOWN((ptr_t)addr, PAGE_SIZE);
		end = ROUND_UP((ptr_t)addr + size, PAGE_SIZE);

		DEBUG(DL_DBG, ("addr(%p), size(%x), base(%x), end(%x)\n",
			       addr, size, base, end));
	
		ASSERT(end > base);
		kmem_unmap(base, end - base, shared);
	}
}
