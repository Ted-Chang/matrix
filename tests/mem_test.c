#include <types.h>
#include <stddef.h>
#include "list.h"
#include "mm/mlayout.h"
#include "mm/page.h"

void mem_test()
{
	u_long *ptr;
	phys_addr_t addr;

	addr = 0x100000;
	ptr = (u_long *)addr;
	*ptr = 0x123456;
	kprintf("value pointed by ptr: 0x%x\n", *ptr);
}
