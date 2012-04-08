#ifndef __PAGE_H__
#define __PAGE_H__

struct page {
	struct list header;		// Link to page queue
	uint64_t addr;			// Physical address of page
	unsigned state;			// State of this page
};

#endif	/* __PAGE_H__ */
