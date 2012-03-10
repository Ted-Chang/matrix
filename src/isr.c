/*
 * isr.c
 */

#include <types.h>
#include "isr.h"
#include "util.h"

isr_t interrupt_handlers[256];

/*
 * Software interrupt handler, call the exception handlers
 */
void isr_handler(struct registers regs)
{
	if (interrupt_handlers[regs.int_no]) {
		isr_t handler = interrupt_handlers[regs.int_no];
		handler(regs);
	} else {
		kprintf("received interrupt: %d\n", regs.int_no);
	}
}

/*
 * Hardware interrupt handler, dispatch the interrupt
 */
void irq_handler(struct registers regs)
{
	/* Send an EOI (end of interrupt) signal to the PICs.
	 * If this interrupt involved the slave.
	 */
	if (regs.int_no >= 40) {
		/* Send reset siginal to slave */
		outportb(0xA0, 0x20);
	}

	/* Send reset signal to master. (As well as slave, if necessary) */
	outportb(0x20, 0x20);

	if (interrupt_handlers[regs.int_no]) {
		isr_t handler = interrupt_handlers[regs.int_no];
		handler(regs);
	}
}

void register_interrupt_handler(uint8_t n, isr_t handler)
{
	interrupt_handlers[n] = handler;
}
