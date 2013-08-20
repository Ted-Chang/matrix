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
	phys_addr_t base, end;

	if (!size) {
		return NULL;
	}

	/* Use the physical map area if the range lies within it. For more
	 * information please refer to the memory layout of our system.
	 */
	if (PMAP_CONTAINS(addr, size)) {
		return (void *)addr;
	}

	/* Not in range of physical map area. Must allocate memory pages and
	 * map there.
	 */
	base = ROUND_DOWN(addr, PAGE_SIZE);
	end = ROUND_UP(addr + size, PAGE_SIZE);

	// TODO: Implement this
	ASSERT(0);

	return NULL;
}

void phys_unmap(void *addr, size_t size, boolean_t shared)
{
	/* If the memory range lies within the physical map area, we don't
	 * need to do anything. Otherwise, unmap and free the kernel memory.
	 */
	if ((addr < KERNEL_PMAP_START) ||
	    (addr > (KERNEL_PMAP_START + KERNEL_PMAP_SIZE))) {
		// TODO: Implement this.
		ASSERT(0);
	}
}
