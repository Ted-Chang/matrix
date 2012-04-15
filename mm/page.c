#include <types.h>
#include <stddef.h>
#include "hal/spinlock.h"
#include "list.h"
#include "cpu.h"
#include "kd.h"
#include "matrix/debug.h"
#include "mm/page.h"
#include "multiboot.h"

#define ABOVE4G			0x100000000LL
#define ABOVE16M		0x1000000LL

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
static struct free_page_list _free_page_lists[NR_FREE_PAGE_LIST];

/* Physical memory ranges */
static struct physical_range _phys_ranges[PHYS_RANG_MAX];
static size_t _nr_phys_ranges = 0;

static kd_status_t kd_cmd_page(int argc, char **argv, struct kd_filter *filter)
{
	return KD_SUCCESS;
}

void page_add_physical_range(phys_addr_t start, phys_addr_t end, uint32_t freelist)
{
	struct free_page_list *list = &_free_page_lists[freelist];
	struct physical_range *range, *prev;

	/* Increase the total page count */
	_nr_total_pages += (end - start) / PAGE_SIZE;

	/* Update the free list to include this range */
	if (!list->minaddr && !list->maxaddr) {
		list->minaddr = start;
		list->maxaddr = end;
	} else {
		if (start < list->minaddr)
			list->minaddr = start;
		if (end > list->maxaddr)
			list->maxaddr = end;
	}

	/* If we are contiguous with the previously recorded range (if any) and
	 * have the same free list index, just append to it, else add a new range.
	 */
	if (_nr_phys_ranges) {
		prev = &_phys_ranges[_nr_phys_ranges - 1];
		if ((start == prev->end) && (freelist == prev->freelist)) {
			prev->end = end;
			return;
		}
	}

	if (_nr_phys_ranges >= PHYS_RANG_MAX) {
		PANIC("Too many physical memory ranges");
	}

	range = &_phys_ranges[_nr_phys_ranges++];
	range->start = start;
	range->end = end;
	range->freelist = freelist;
}

void platform_init_page()
{
	struct multiboot_mmap_entry *mmap;
	
	for (mmap = _mbi->mmap_addr;
	     (unsigned long)mmap < (_mbi->mmap_addr + _mbi->mmap_length);
	     mmap = (struct multiboot_mmap_entry *)
		     ((u_long)mmap + mmap->size + sizeof(mmap->size))) {
		switch (mmap->type) {
		case MULTIBOOT_MEMORY_AVAILABLE:
		case MULTIBOOT_MEMORY_RESERVED:
			break;
		default:
			continue;
		}

		/* Determine which free list pages in the range should be put in.
		 * If necessary, split into multiple ranges.
		 */
		if (mmap->addr < ABOVE16M) {
			if (mmap->addr + mmap->len <= ABOVE16M) {
				page_add_physical_range(mmap->addr, mmap->addr + mmap->size,
							FREE_PAGE_LIST_BELOW16M);
			} else if (mmap->addr + mmap->len <= ABOVE4G) {
				page_add_physical_range(mmap->addr, ABOVE16M,
							FREE_PAGE_LIST_BELOW16M);
				page_add_physical_range(ABOVE16M, mmap->addr + mmap->size,
							FREE_PAGE_LIST_BELOW4G);
			} else {
				page_add_physical_range(mmap->addr, ABOVE16M,
							FREE_PAGE_LIST_BELOW16M);
				page_add_physical_range(ABOVE16M, ABOVE4G,
							FREE_PAGE_LIST_BELOW4G);
				page_add_physical_range(ABOVE4G, mmap->addr + mmap->size,
							FREE_PAGE_LIST_ABOVE4G);
			}
		} else if (mmap->addr < ABOVE4G) {
			if (mmap->addr + mmap->len <= ABOVE4G) {
				page_add_physical_range(mmap->addr, mmap->addr + mmap->size,
							FREE_PAGE_LIST_BELOW4G);
			} else {
				page_add_physical_range(mmap->addr, ABOVE4G,
							FREE_PAGE_LIST_BELOW4G);
				page_add_physical_range(ABOVE4G, mmap->addr + mmap->size,
							FREE_PAGE_LIST_ABOVE4G);
			}
		} else {
			page_add_physical_range(mmap->addr, mmap->addr + mmap->size,
						FREE_PAGE_LIST_ABOVE4G);
		}
	}
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
		LIST_INIT(&_free_page_lists[i].pages);
		_free_page_lists[i].minaddr = 0;
		_free_page_lists[i].maxaddr = 0;
	}

	/* First call into platform-specific code to parse the memory map
	 * provided by the loader and separate it further as required
	 */
	platform_init_page();
	DEBUG(DL_DBG, ("page: available physical memory ranges:\n"));
	for (i = 0; i < _nr_phys_ranges; i++) {
		DEBUG(DL_DBG, ("0x%x - 0x%x [%d]\n",
			       _phys_ranges[i].start, _phys_ranges[i].end,
			       _phys_ranges[i].freelist));
	}
	DEBUG(DL_DBG, ("page: free list coverage:\n"));
	for (i = 0; i < NR_FREE_PAGE_LIST; i++) {
		DEBUG(DL_DBG, ("%d: 0x%x - 0x%x\n", i,
			       _free_page_lists[i].minaddr,
			       _free_page_lists[i].maxaddr));
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
