#ifndef __PAGE_H__
#define __PAGE_H__

#ifdef _X86_
#define PAGE_SIZE		4096	// Size of a page (4KB)
#endif	/* _X86_ */

typedef uint32_t page_num_t;

/*
 * Page Table Entry
 */
struct page {
	uint32_t present:1;		// Page present in memory
	uint32_t rw:1;			// Read-only if cleared, read-write if set
	uint32_t user:1;		// Supervisor level only if clear
	uint32_t accessed:1;		// Has the page been accessed since last refresh
	uint32_t dirty:1;		// Has the page been written to since last refresh
	uint32_t reserved:7;		// Reserved bits
	uint32_t frame:20;		// Frame address
};

extern phys_addr_t _placement_addr;

extern void page_alloc(struct page *p, int flags);
extern void page_free(struct page *p);
extern void init_page();

#endif	/* __PAGE_H__ */
