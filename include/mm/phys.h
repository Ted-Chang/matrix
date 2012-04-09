#ifndef __PHYS_H__
#define __PHYS_H__

extern void *physical_map(uint32_t addr, size_t size, int mmflag);
extern void physical_unmap(void *addr, size_t size, boolean_t shared);

#endif	/* __PHYS_H__ */
