/*
 * isr.c
 */

#include "types.h"
#include "isr.h"
#include "util.h"

void isr_handler(struct registers regs)
{
	kprintf("received interrupt: %d\n", regs.int_no);
}
