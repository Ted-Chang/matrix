#ifndef __PHYS_H__
#define __PHYS_H__

extern void *phys_map(phys_addr_t addr, size_t size, int mmflag);
extern void phys_unmap(void *addr, size_t size, boolean_t shared);

#endif	/* __PHYS_H__ */
