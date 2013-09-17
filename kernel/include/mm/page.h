#ifndef __PAGE_H__
#define __PAGE_H__

#ifdef _X86_
#define PAGE_SIZE	(4096)	// Size of a page (4KB)
#endif	/* _X86_ */

typedef uint32_t page_num_t;

/*
 * X86 Page Table Entry
 */
struct page {
	uint32_t present:1;	// Page present in memory
	uint32_t rw:1;		// Read/Write; if 0, writes may not be
				// allowed(depends on CPL and CR0.WP)
	
	uint32_t user:1;	// Supervisor level only if clear
	uint32_t pwt:1;		// Page-level write through
	uint32_t pcd:1;		// Page-level cache disabled
	uint32_t accessed:1;	// Accessed; indicate whether software
				// has accessed it
	
	uint32_t dirty:1;	// Dirty; indicate whether software has
				// written to it
	
	uint32_t pat:1;		// If PAT is supported, indirectly determine
				// memory type
	
	uint32_t global:1;	// Global; if CR4.PGE = 1, determines whether
				// the translation is global
	
	uint32_t reserved:3;	// Reserved bits
	uint32_t frame:20;	// Frame address
};

extern void page_early_alloc(phys_addr_t *phys, size_t size, boolean_t align);
extern void page_alloc(struct page *p, int flags);
extern void page_free(struct page *p);
extern void page_copy(phys_addr_t dst, phys_addr_t src);
extern void init_page();

#endif	/* __PAGE_H__ */
