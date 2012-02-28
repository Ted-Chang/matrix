#ifndef __MMGR_H__
#define __MMGR_H__

#include <isr.h>	// For struct registers

/*
 * Page Table Entry
 */
typedef struct pte {
	uint32_t present:1;	// Page present in memory
	uint32_t rw:1;		// Read-only if cleared, read-write if set
	uint32_t user:1;	// Supervisor level only if clear
	uint32_t accessed:1;	// Has the page been accessed since last refresh
	uint32_t dirty:1;	// Has the page been written to since last refresh
	uint32_t reserved:7;	// Reserved bits
	uint32_t frame:20;	// Frame address
} pte_t;

/*
 * Page Table
 * Each page table has 1024 page table entries, actually each
 * page table entry is just a 32 bit integer for X86 arch
 */
typedef struct pt {
	struct pte pages[1024];
} pt_t;

/*
 * Page Directory
 * Each page directory has 1024 pointers to its page table entry
 */
typedef struct pd {
	struct pt *tables[1024];

	/* Array of pointers to the page tables above, but gives their
	 * physical location, for loading into the CR3 register.
	 */
	uint32_t physical_tables[1024];
	
	/* The physical address of physical_tables. This comes into
	 * play when we get our kernel heap allocated and the directory
	 * may be in a different location in virtual memory.
	 */
	uint32_t physical_addr;
} pd_t;


void init_paging();

void switch_page_dir(struct pd *new);

struct pte *get_pte(uint32_t addr, int make, struct pd *dir);

void page_fault(struct registers regs);

struct pd *clone_pd(struct pd *src);

#endif	/* __MMGR_H__ */
