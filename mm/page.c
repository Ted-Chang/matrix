#include <types.h>
#include <stddef.h>
#include <string.h>
#include "hal/spinlock.h"
#include "list.h"
#include "cpu.h"
#include "kd.h"
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "mm/mm.h"
#include "mm/page.h"
#include "mm/mlayout.h"
#include "multiboot.h"

#define ABOVE4G			0x100000000LL
#define ABOVE16M		0x1000000LL

/* Number of the page queues */
#define NR_PAGE_QUEUE		3

/* Maximum number of physical ranges */
#define PHYS_ZONE_MAX		32

/* Structure describing a range of physical memory. */
struct physical_zone {
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
static struct physical_zone _phys_zones[PHYS_ZONE_MAX];
static size_t _nr_phys_zones = 0;

static kd_status_t kd_cmd_page(int argc, char **argv, struct kd_filter *filter)
{
	return KD_SUCCESS;
}

static inline void page_queue_append(u_long index, struct page *page)
{
	ASSERT(LIST_EMPTY(&page->header));

	spinlock_acquire(&_page_queues[index].lock);
	list_add_tail(&page->header, &_page_queues[index].pages);
	_page_queues[index].count++;
	spinlock_release(&_page_queues[index].lock);
}

static inline void page_queue_remove(u_long index, struct page *page)
{
	spinlock_acquire(&_page_queues[index].lock);
	list_del(&page->header);
	_page_queues[index].count--;
	spinlock_release(&_page_queues[index].lock);
}

struct page *page_lookup(phys_addr_t addr)
{
	size_t i;

	ASSERT(!(addr % PAGE_SIZE));

	for (i = 0; i < _nr_phys_zones; i++) {
		if (addr >= _phys_zones[i].start && addr < _phys_zones[i].end) {
			return &_phys_zones[i].pages[(addr - _phys_zones[i].start) >> PAGE_WIDTH];
		}
	}
	
	return NULL;
}

void page_add_physical_zone(phys_addr_t start, phys_addr_t end, uint32_t freelist)
{
	struct free_page_list *list = &_free_page_lists[freelist];
	struct physical_zone *zone, *prev;

	/* Increase the total page count */
	_nr_total_pages += ((end - start) / PAGE_SIZE);

	/* Update the free list to include this zone */
	if (!list->minaddr && !list->maxaddr) {		// First initialization 
		list->minaddr = start;
		list->maxaddr = end;
	} else {
		if (start < list->minaddr)
			list->minaddr = start;
		if (end > list->maxaddr)
			list->maxaddr = end;
	}

	/* If we are contiguous with the previously recorded zone (if any) and
	 * have the same free list index, just append to it, else add a new zone.
	 */
	if (_nr_phys_zones) {
		prev = &_phys_zones[_nr_phys_zones - 1];
		if ((start == prev->end) && (freelist == prev->freelist)) {
			prev->end = end;
			return;
		}
	}

	if (_nr_phys_zones >= PHYS_ZONE_MAX) {
		PANIC("Too many physical memory zones");
	}

	zone = &_phys_zones[_nr_phys_zones++];
	zone->start = start;
	zone->end = end;
	zone->freelist = freelist;
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

		/* Determine which free list pages in the zone should be put in.
		 * If necessary, split into multiple zones.
		 */
		if (mmap->addr < ABOVE16M) {
			if (mmap->addr + mmap->len <= ABOVE16M) {
				page_add_physical_zone(mmap->addr, mmap->addr + mmap->len,
						       FREE_PAGE_LIST_BELOW16M);
			} else if (mmap->addr + mmap->len <= ABOVE4G) {
				page_add_physical_zone(mmap->addr, ABOVE16M,
						       FREE_PAGE_LIST_BELOW16M);
				page_add_physical_zone(ABOVE16M, mmap->addr + mmap->len,
						       FREE_PAGE_LIST_BELOW4G);
			} else {
				page_add_physical_zone(mmap->addr, ABOVE16M,
						       FREE_PAGE_LIST_BELOW16M);
				page_add_physical_zone(ABOVE16M, ABOVE4G,
						       FREE_PAGE_LIST_BELOW4G);
				page_add_physical_zone(ABOVE4G, mmap->addr + mmap->len,
						       FREE_PAGE_LIST_ABOVE4G);
			}
		} else if (mmap->addr < ABOVE4G) {
			if (mmap->addr + mmap->len <= ABOVE4G) {
				page_add_physical_zone(mmap->addr, mmap->addr + mmap->len,
						       FREE_PAGE_LIST_BELOW4G);
			} else {
				page_add_physical_zone(mmap->addr, ABOVE4G,
							FREE_PAGE_LIST_BELOW4G);
				page_add_physical_zone(ABOVE4G, mmap->addr + mmap->len,
						       FREE_PAGE_LIST_ABOVE4G);
			}
		} else {
			page_add_physical_zone(mmap->addr, mmap->addr + mmap->len,
					       FREE_PAGE_LIST_ABOVE4G);
		}
	}
}

void init_page()
{
	struct multiboot_mmap_entry *mmap;
	phys_addr_t pages_base = 0, offset = 0, addr;
	phys_size_t pages_size = 0, size;
	page_num_t count, j;
	struct page *page;
	boolean_t free;
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
	DEBUG(DL_DBG, ("page: available physical memory zones:\n"));
	for (i = 0; i < _nr_phys_zones; i++) {
		DEBUG(DL_DBG, ("0x%016lx - 0x%016lx [%d]\n",
			       _phys_zones[i].start, _phys_zones[i].end,
			       _phys_zones[i].freelist));
	}
	DEBUG(DL_DBG, ("page: free list coverage:\n"));
	for (i = 0; i < NR_FREE_PAGE_LIST; i++) {
		DEBUG(DL_DBG, ("%d: 0x%016lx - 0x%016lx\n", i,
			       _free_page_lists[i].minaddr,
			       _free_page_lists[i].maxaddr));
	}

	/* Find a suitable free zone to store the page structures in. This zone
	 * must be the largest possible zone that is accessible through the physical
	 * map area, as we cannot map in any new memory currently.
	 */
	for (mmap = _mbi->mmap_addr;
	     (u_long)mmap < (_mbi->mmap_addr + _mbi->mmap_length);
	     mmap = (struct multiboot_mmap_entry *)
		     ((u_long)mmap + mmap->size + sizeof(mmap->size))) {
		/* Only available memory can be used */
		if (mmap->type != MULTIBOOT_MEMORY_AVAILABLE)
			continue;

		if (mmap->addr < KERNEL_PMAP_SIZE) {
			size = MIN(KERNEL_PMAP_SIZE, mmap->len);
			if (size > pages_size) {
				pages_base = mmap->addr;
				pages_size = size;
			}
		}
	}

	/* Check if we have enough memory in this zone. */
	size = ROUND_UP(sizeof(struct page) * _nr_total_pages, PAGE_SIZE);
	if (size > pages_size) {
		PANIC("Not enough contiguous memory for initialization");
	}
	pages_size = size;

	DEBUG(DL_DBG, ("page: %d pages, %dKB was used for structures at 0x%016lx\n",
		       _nr_total_pages, pages_size / 1024, pages_base));
	
	/* Create page structures for each memory zone we have */
	for (i = 0; i < _nr_phys_zones; i++) {
		/* Calculate how many bytes we need for this zone */
		count = (_phys_zones[i].end - _phys_zones[i].start) / PAGE_SIZE;
		size = sizeof(struct page) * count;
		/* Map the memory for it */
		_phys_zones[i].pages = phys_map(pages_base + offset, size, MM_BOOT);
		offset += size;
		
		/* Initialize the pages of this zone */
		memset(_phys_zones[i].pages, 0, size);
		for (j = 0; j < count; j++) {
			LIST_INIT(&_phys_zones[i].pages[j].header);
			_phys_zones[i].pages[j].addr = _phys_zones[i].start +
				((phys_addr_t)j * PAGE_SIZE);
			_phys_zones[i].pages[j].phys_zone = i;

			ASSERT(LIST_EMPTY(&_phys_zones[i].pages[j].header));
		}
	}

	/* Finally, set the state of each page */
	for (mmap = _mbi->mmap_addr;
	     (u_long)mmap < (_mbi->mmap_addr + _mbi->mmap_length);
	     mmap = (struct multiboot_mmap_entry *)
		     ((u_long)mmap + mmap->size + sizeof(mmap->size))) {
		/* Determine what state to give pages in this zone */
		switch (mmap->type) {
		case MULTIBOOT_MEMORY_AVAILABLE:
			free = TRUE;
			break;
		case MULTIBOOT_MEMORY_RESERVED:
			free = FALSE;
			break;
		default:
			continue;
		}

		/* We must individually look up each page because a signle memory map
		 * can be split over multiple physical zone.
		 */
		for (addr = mmap->addr;
		     addr != (mmap->addr + mmap->len);
		     addr += PAGE_SIZE) {
			page = page_lookup(addr);
			ASSERT(page);

			/* If the page was used for the page structures, mark it as
			 * allocated.
			 */
			if (!free ||
			    (addr >= pages_base && addr < (pages_base + pages_size))) {
				page->state = PAGE_STATE_ALLOCATED;
				page_queue_append(PAGE_STATE_ALLOCATED, page);
			} else {
				page->state = PAGE_STATE_FREE;
				list_add_tail(&page->header,
					      &_free_page_lists[_phys_zones[page->phys_zone].freelist].pages);
			}
		}
	}
	
	/* Register the kernel debugger command */
	kd_register_cmd("page", "Display physical memory usage information",
			kd_cmd_page);
}
