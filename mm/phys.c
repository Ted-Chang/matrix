#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "list.h"
#include "mm/page.h"
#include "mm/mlayout.h"
#include "mm/mm.h"
#include "mm/phys.h"
#include "mm/kmem.h"

#define PMAP_CONTAINS(addr, size) \
	(addr >= KERNEL_PMAP_OFFSET && \
	 (addr + size) <= (KERNEL_PMAP_OFFSET + KERNEL_PMAP_SIZE))

void *physical_map(uint32_t addr, size_t size, int mmflag)
{
	uint64_t base, end;

	if (!size)
		return NULL;

	/* Use the physical map area if the range lies within it */
	if (PMAP_CONTAINS(addr, size))
		return (void *)(KERNEL_PMAP_BASE +
				(addr - KERNEL_PMAP_OFFSET));

	/* Outside the physical map area. Must allocate some kernel memory
	 * and map there instead.
	 */
	base = ROUND_DOWN((uint32_t)addr, PAGE_SIZE);
	end = ROUND_UP((uint32_t)addr + size, PAGE_SIZE);
	
	return kmem_map(base, end-base, mmflag);
}

void physical_unmap(void *addr, size_t size, boolean_t shared)
{
	uint64_t base, end;

	/* If the range lies within the physical map area, don't need to do
	 * anything. Otherwise, unmap and free from kernel memory.
	 */
	if (addr < KERNEL_PMAP_BASE ||
	    addr >= (KERNEL_PMAP_BASE + KERNEL_PMAP_SIZE)) {
		base = ROUND_DOWN((uint32_t)addr, PAGE_SIZE);
		end = ROUND_UP((uint32_t)addr + size, PAGE_SIZE);

		kmem_unmap((void *)base, end-base, shared);
	}
}
