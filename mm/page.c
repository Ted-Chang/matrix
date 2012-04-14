#include <types.h>
#include <stddef.h>
#include "hal/spinlock.h"
#include "list.h"
#include "cpu.h"
#include "kd.h"
#include "matrix/debug.h"
#include "mm/page.h"

/* Number of the page queues */
#define NR_PAGE_QUEUE		3

/* Maximum number of physical ranges */
#define PHYS_RANG_MAX		32

/* Structure describing a range of physical memory. */
struct physical_range {
	phys_addr_t start;		// Start of the range
	phys_addr_t end;		// End of the range
	struct page *pages;		// Pages in the range
	uint32_t freelist;		// Free page list index
};

/* Structure describing a page queue */
struct page_queue {
	struct list pages;		// List of pages
	page_num_t count;		// Number of pages in the queue
	struct spinlock lock;		// Lock to protect the queue
};

/* Structure describing a free page list */
struct free_page_list {
	struct list pages;		// Pages in the list
	phys_addr_t minaddr;		// Lowest start address contained in the list
	phys_addr_t maxaddr;		// Highest end address contained in the list
};

/* Total usable pages */
static page_num_t _nr_total_pages = 0;

/* Allocated page queues */
static struct page_queue _page_queues[NR_PAGE_QUEUE];

/* Free page list */
static struct free_page_list _free_page_list[NR_FREE_PAGE_LIST];

/* Physical memory ranges */
static struct physical_range _phys_ranges[PHYS_RANG_MAX];
static size_t _nr_phys_ranges = 0;

static kd_status_t kd_cmd_page(int argc, char **argv, struct kd_filter *filter)
{
	return KD_SUCCESS;
}

void page_add_physical_range(phys_addr_t start, phys_addr_t end, unsigned freelist)
{
	;
}

void platform_init_page()
{
	;
}

void init_page()
{
	phys_addr_t pages_base = 0, offset = 0, addr;
	phys_addr_t pages_size = 0, size;
	page_num_t count, j;
	size_t i;

	/* Initialize page queues and freelists */
	for (i = 0; i < NR_PAGE_QUEUE; i++) {
		LIST_INIT(&_page_queues[i].pages);
		_page_queues[i].count = 0;
		spinlock_init(&_page_queues[i].lock, "page_queue_lock");
	}
	for (i = 0; i < NR_FREE_PAGE_LIST; i++) {
		LIST_INIT(&_free_page_list[i].pages);
		_free_page_list[i].minaddr = 0;
		_free_page_list[i].maxaddr = 0;
	}

	/* First call into platform-specific code to parse the memory map
	 * provided by the loader and separate it further as required
	 */
	platform_init_page();
	DEBUG(DL_DBG, ("page: available physical memory ranges:\n"));
	for (i = 0; i < _nr_phys_ranges; i++) {
		DEBUG(DL_DBG, ("0x%x - 0x%x [%u]\n",
			       _phys_ranges[i].start, _phys_ranges[i].end,
			       _phys_ranges[i].freelist));
	}
	DEBUG(DL_DBG, ("page: free list coverage:\n"));
	for (i = 0; i < NR_FREE_PAGE_LIST; i++) {
		DEBUG(DL_DBG, ("%d: 0x%x - 0x%x\n", i,
			       _free_page_list[i].minaddr,
			       _free_page_list[i].maxaddr));
	}

	DEBUG(DL_DBG, ("page: %d pages, %dKB was used for structures at 0x%x\n",
		       _nr_total_pages, pages_size / 1024, pages_base));

	/* Create page structures for each memory range we have */
	for (i = 0; i < _nr_phys_ranges; i++) {
		count = (_phys_ranges[i].end - _phys_ranges[i].start) / PAGE_SIZE;
		size = sizeof(struct page) * count;
		
		/* Initialize each range */
		memset(_phys_ranges[i].pages, 0, size);
		for (j = 0; j < count; j++) {
			LIST_INIT(&_phys_ranges[i].pages[j].header);
			_phys_ranges[i].pages[j].addr = _phys_ranges[i].start +
				((phys_addr_t)j * PAGE_SIZE);
			_phys_ranges[i].pages[j].phys_range = i;
		}
	}
	
	/* Register the kernel debugger command */
	kd_register_cmd("page", "Display physical memory usage information",
			kd_cmd_page);
}
