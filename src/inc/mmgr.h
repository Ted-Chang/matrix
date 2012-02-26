#ifndef __MMGR_H__
#define __MMGR_H__

#include <isr.h>	// For struct registers

/*
 * Page Table Entry
 */
typedef struct pte {
	uint32_t present:1;
	uint32_t rw:1;
	uint32_t user:1;
	uint32_t accessed:1;
	uint32_t dirty:1;
	uint32_t unused:7;
	uint32_t frame:20;
} pte_t;

/*
 * Page Table
 */
typedef struct pt {
	struct pte pages[1024];
} pt_t;

/*
 * Page Directory
 */
typedef struct pd {
	struct pt *tables[1024];
	uint32_t physical_tables[1024];
	uint32_t physical_addr;
} pd_t;

void init_paging();

void switch_page_dir(struct pd *new);

struct pte *get_pte(uint32_t addr, int make, struct pd *dir);

void page_fault(struct registers regs);

struct pd *clone_pd(struct pd *src);

#endif	/* __MMGR_H__ */
