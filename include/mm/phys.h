#ifndef __PHYS_H__
#define __PHYS_H__

/* Memory zone types */
#define MEM_TYPE_NORMAL		0	// Normal memory
#define MEM_TYPE_DEVICE		1	// Device memory
#define MEM_TYPE_UC		2	// Uncacheable
#define MEM_TYPE_WC		3	// Write Combining
#define MEM_TYPE_WT		4	// Write through
#define MEM_TYPE_WB		5	// Write Back

extern void *phys_map(phys_addr_t addr, size_t size, int mmflag);
extern void phys_unmap(void *addr, size_t size, boolean_t shared);

#endif	/* __PHYS_H__ */
