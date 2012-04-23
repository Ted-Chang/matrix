#include <types.h>
#include <stddef.h>
#include "list.h"
#include "hal/spinlock.h"
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "mm/page.h"
#include "mm/mlayout.h"
#include "mm/mm.h"
#include "mm/phys.h"
#include "mm/kmem.h"

#define PMAP_CONTAINS(addr, size)	((addr + size) <= KERNEL_PMAP_SIZE)

/* Structure containing a memory type zone */
struct mem_type_zone {
	phys_addr_t start;	// Start of a range
	phys_addr_t end;	// End of a range
	uint32_t type;		// Type of the range
};

/* Maximum number of memory type ranges */
#define MEM_TYPE_ZONE_MAX	64

/* Memory type ranges */
static struct mem_type_zone _mem_types[MEM_TYPE_ZONE_MAX];
static size_t _nr_mem_types = 0;
struct spinlock _mem_types_lock;

void *phys_map(phys_addr_t addr, size_t size, int mmflag)
{
	phys_addr_t base, end;

	if (!size)
		return NULL;

	/* Use the physical map area if the zone lies within it */
	if (PMAP_CONTAINS(addr, size))
		return (void *)(KERNEL_PMAP_BASE + (addr - KERNEL_PMAP_OFFSET));

	ASSERT(FALSE);
	
	return NULL;
}

void phys_unmap(void *addr, size_t size, boolean_t shared)
{
	;
}

boolean_t phys_copy(phys_addr_t dest, phys_addr_t src, int mmflag)
{
	void *mdest, *msrc;

	ASSERT(!(dest % PAGE_SIZE));
	ASSERT(!(src % PAGE_SIZE));

	return TRUE;
}

uint32_t phys_mem_type(phys_addr_t addr)
{
	size_t count, i;

	/* We do not take the lock here: doing so would mean an additional
	 * spinlock acquisition for every memory mapping operation. Instead
	 * of that we just take the current count and iterate up to it.
	 */
	for (count = _nr_mem_types, i = 0; i < count; i++) {
		if (addr >= _mem_types[i].start && addr < _mem_types[i].end)
			return _mem_types[i].type;
	}

	return MEM_TYPE_NORMAL;
}

void phys_set_mem_type(phys_addr_t addr, phys_size_t size, uint32_t type)
{
	ASSERT(!(addr & PAGE_SIZE));
	ASSERT(!(size & PAGE_SIZE));

	spinlock_acquire(&_mem_types_lock);
	
	if (_nr_mem_types >= MEM_TYPE_ZONE_MAX)
		PANIC("Too many phys_set_mem_type() calls");

	_mem_types[_nr_mem_types].start = addr;
	_mem_types[_nr_mem_types].end = addr + size;
	_mem_types[_nr_mem_types].type = type;
	_nr_mem_types++;
	
	spinlock_release(&_mem_types_lock);
}
