#include <types.h>
#include <stddef.h>
#include "list.h"
#include "mm/mm.h"
#include "mm/mlayout.h"
#include "mm/kmem.h"

/* Number of free list, it is the bits of a pointer in current architecture */
#define KMEM_FREELISTS		32

/* Initial hash table size */
#define KMEM_INITIAL_HASH_SIZE	16

/* Depth of a hash chain at which a rehash will be triggered */
#define KMEM_REHASH_THRESHOLD	32

/* Kernel memory range structure */
struct kmem_range {
	struct list range_link;
	struct list af_link;

	uint32_t addr;
	size_t size;
	boolean_t allocated;
};

/* Initial allocation hash table */
static struct list kmem_initial_hash[KMEM_INITIAL_HASH_SIZE];

/* Allocation hash table data */
static struct list *kmem_hash = kmem_initial_hash;
static size_t kmem_hash_size = KMEM_INITIAL_HASH_SIZE;
static boolean_t kmem_rehash_requested = FALSE;

/* Free range list data */
static struct list kmem_freelists[KMEM_FREELISTS];
static uint32_t kmem_freemap = 0;

/* Sorted list of all kernel memory ranges */
static struct list kmem_ranges;

/* Pool of free range structures */
static struct list kmem_range_pool;

static struct kmem_range *kmem_range_get(int mmflag)
{
	struct kmem_range *range, *first;
	uint64_t page;
	size_t i;

	if (LIST_EMPTY(&kmem_range_pool)) {
		/* No free range structure available. Allocate a new page that
		 * can be accessed from the physical map area. It is expected
		 * that the architecture segregates the free page lists such that
		 * pages accessible through the physical map area can be allocated
		 * using the fast path, and are not allocated unless pages outside
		 * of it aren't available.
		 */
		;
	} else {
		/* Pop a structure off the list */
		;
	}
}

void *kmem_map(uint64_t base, size_t size, int mmflag)
{
	return NULL;
}

void kmem_unmap(void *addr, size_t size, boolean_t shared)
{
	;
}

void init_kmem()
{
	struct kmem_range *range;
	unsigned i;

	/* Initialize the lists */
	for (i = 0; i < KMEM_INITIAL_HASH_SIZE; i++)
		LIST_INIT(&kmem_initial_hash[i]);

	for (i = 0; i < KMEM_FREELISTS; i++)
		LIST_INIT(&kmem_freelists[i]);

	/* Create the initial free range. */
	range = kmem_range_get(MM_BOOT);
	range->addr = KERNEL_KMEM_BASE;
	range->size = KERNEL_KMEM_SIZE;
}
