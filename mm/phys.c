#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "list.h"
#include "mm/page.h"
#include "mm/mlayout.h"
#include "mm/mm.h"
#include "mm/phys.h"
#include "mm/kmem.h"
#include "hal/spinlock.h"

#define PMAP_CONTAINS(addr, size) \
	(addr >= KERNEL_PMAP_OFFSET && \
	 (addr + size) <= (KERNEL_PMAP_OFFSET + KERNEL_PMAP_SIZE))

/* Structure containing a memory type range */
struct mem_type_range {
	phys_addr_t start;	// Start of a range
	phys_addr_t end;	// End of a range
	uint32_t type;		// Type of the range
};

/* Maximum number of memory type ranges */
#define MEM_TYPE_ZONE_MAX	64

/* Memory type ranges */
static struct mem_type_range _mem_types[MEM_TYPE_ZONE_MAX];
static size_t _mem_types_count = 0;
struct spinlock _mem_types_lock;

void *phys_map(phys_addr_t addr, size_t size, int mmflag)
{
	phys_addr_t base, end;

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

void phys_unmap(void *addr, size_t size, boolean_t shared)
{
	phys_addr_t base, end;

	/* If the range lies within the physical map area, don't need to do
	 * anything. Otherwise, unmap and free from kernel memory.
	 */
	if ((uint32_t)addr < KERNEL_PMAP_BASE ||
	    (uint32_t)addr >= (KERNEL_PMAP_BASE + KERNEL_PMAP_SIZE)) {
		base = ROUND_DOWN((uint32_t)addr, PAGE_SIZE);
		end = ROUND_UP((uint32_t)addr + size, PAGE_SIZE);

		kmem_unmap((void *)base, end-base, shared);
	}
}
