/*
 * isr.c
 */

#include <types.h>
#include "hal.h"
#include "isr.h"
#include "util.h"

isr_t _interrupt_handlers[256];

/*
 * Software interrupt handler, call the exception handlers
 */
void isr_handler(struct registers regs)
{
	/* Avoid the problem caused by the signed interrupt number if it is
	 * max than 0x80
	 */
	uint8_t int_no = regs.int_no & 0xFF;

	if (_interrupt_handlers[int_no]) {
		isr_t handler = _interrupt_handlers[int_no];
		handler(&regs);
	} else {
		kprintf("unhandled interrupt: %d\n", int_no);
		for (; ; ) ;
	}
}

/*
 * Hardware interrupt handler, dispatch the interrupt
 */
void irq_handler(struct registers regs)
{
	/* Notify the PIC that we have done so we can accept >= priority IRQs now */
	interrupt_done(regs.int_no);

	if (_interrupt_handlers[regs.int_no]) {
		isr_t handler = _interrupt_handlers[regs.int_no];
		handler(&regs);
	}
}

void register_interrupt_handler(uint8_t n, isr_t handler)
{
	_interrupt_handlers[n] = handler;
}
