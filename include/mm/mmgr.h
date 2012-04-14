/*
 * mmgr.h
 */

#ifndef __MMGR_H__
#define __MMGR_H__

#include "hal/isr.h"

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

extern void init_kheap(uint64_t mem_size);
extern void switch_page_dir(struct pd *new);
extern struct pte *get_pte(uint32_t addr, int make, struct pd *dir);
extern void page_fault(struct intr_frame *frame);
extern struct pd *clone_pd(struct pd *src);
extern void alloc_frame(struct pte *pte, int is_kernel, int is_writable);
extern void free_frame(struct pte *pte);
extern uint32_t kmalloc(size_t size);
extern uint32_t kmalloc_a(size_t size);
extern void *physical_map(phys_addr_t addr, size_t size, int flags);

#endif	/* __MMGR_H__ */
