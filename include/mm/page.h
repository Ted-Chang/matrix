#ifndef __PAGE_H__
#define __PAGE_H__

#include "list.h"

#ifdef _X86_
#define PAGE_WIDTH		12	// Width of a page in bits
#define PAGE_SIZE		4096	// Size of a page (4KB)

/* Mask to clear page offset and unsupported bits from a virtual address */
#define PAGE_MASK		0xFFFFF000LL

/* Mask to clear page offset and unsupported bits from a physical address */
#define PHYS_PAGE_MASK		0xFFFFF000LL
#endif	/* _X86_ */

#define NR_FREE_PAGE_LIST	3

/**
 * Free page list number definitions.
 * On the PC, we split into 3 lists: below 16MB (for ISA DMA), below 4GB(for
 * devices needing 32-bit DMA addresses) and the rest. Since the page allocater
 * will search the lists from lowest index to highest, we place over 4GB first,
 * then below 4GB, then 16MB. 
 */
#define FREE_PAGE_LIST_ABOVE4G	0
#define FREE_PAGE_LIST_BELOW4G	1
#define FREE_PAGE_LIST_BELOW16M	2

typedef uint32_t page_num_t;

/* Structure describing physical memory usage statistics */
struct page_stats {
	uint64_t total;			// Total available memory
	uint64_t allocated;		// Amount of memory in use
	uint64_t modified;		// Amount of memory containing modified data
	uint64_t cached;		// Amount of memory being used by caches
	uint64_t free;			// Amount of free memory
};

/* Structure describing a page in memory */
struct page {
	struct list header;		// Link to page queue

	/* Basic page information */
	phys_addr_t addr;		// Physical address of page
	uint32_t phys_zone;		// Memory zone that the page belongs to
	uint32_t state;			// State of this page
	uint8_t modified:1;		// Whether the page has been modified
	uint8_t reserved:7;
};

/* Possible state of a page */
#define PAGE_STATE_ALLOCATED	0
#define PAGE_STATE_MODIFIED	1
#define PAGE_STATE_CACHED	2
#define PAGE_STATE_FREE		3

extern struct page *page_alloc(int mmflag);
extern void page_free(struct page *page);
extern void page_stats_get(struct page_stats *stats);
extern void init_page();

#endif	/* __PAGE_H__ */
