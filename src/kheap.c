#include "types.h"
#include "kheap.h"

extern uint32_t end;	// end is defined in linker scripts

uint32_t placement_addr = (uint32_t)&end;

uint32_t kmalloc_int(uint32_t size, int align, uint32_t *phys)
{
	uint32_t tmp;
	
	if ((align == 1) && (placement_addr & 0xFFFFF000)) {
		placement_addr &= 0xFFFFF000;
		placement_addr += 0x1000;
	}

	if (phys) {
		*phys = placement_addr;
	}

	tmp = placement_addr;
	placement_addr += size;

	return tmp;
}

uint32_t kmalloc(uint32_t size)
{
	return kmalloc_int(size, 0, 0);
}

uint32_t kmalloc_a(uint32_t size)
{
	return kmalloc_int(size, 1, 0);
}

uint32_t kmalloc_p(uint32_t size, uint32_t *phys)
{
	return kmalloc_int(size, 0, phys);
}

uint32_t kmalloc_ap(uint32_t size, uint32_t *phys)
{
	return kmalloc_int(size, 1, phys);
}
