/*
 * isr.c
 */

#include <types.h>
#include <stddef.h>
#include "hal/isr.h"
#include "hal/hal.h"
#include "hal/lirq.h"
#include "util.h"
#include "matrix/debug.h"

struct irq_hook *_interrupt_handlers[256];

/*
 * Software interrupt handler, call the exception handlers
 */
void isr_handler(struct intr_frame frame)
{
	struct irq_hook *hook;
	boolean_t processed = FALSE;
	
	/* Avoid the problem caused by the signed interrupt number if it is
	 * max than 0x80
	 */
	uint8_t int_no = frame.int_no & 0xFF;

	hook = _interrupt_handlers[int_no];
	while (hook) {
		isr_t handler = hook->handler;
		if (handler)
			handler(&frame);
		hook = hook->next;
		processed = TRUE;
	}

	if (!processed) {
		kprintf("unhandled interrupt: %d\n", int_no);
		for (; ; ) ;
	}
}

/*
 * Hardware interrupt handler, dispatch the interrupt
 */
void irq_handler(struct intr_frame frame)
{
	struct irq_hook *hook;
	
	/* Notify the PIC that we have done so we can accept >= priority IRQs now */
	interrupt_done(frame.int_no);

	hook = _interrupt_handlers[frame.int_no];
	while (hook) {
		isr_t handler = hook->handler;
		if (handler)
			handler(&frame);
		hook = hook->next;
	}
}

void register_interrupt_handler(uint8_t irq, struct irq_hook *hook, isr_t handler)
{
	struct irq_hook **line;
	
	if (irq < 0 || irq >= 256)
		PANIC("register_interrupt_handler: invalid irq!\n");
	
	irq_disable();

	line = &_interrupt_handlers[irq];
	while (*line) {
		/* Check if the hook has been registered already */
		if (hook == (*line)) {
			irq_enable();
			return;
		}
		line = &((*line)->next);
	}

	hook->next = NULL;
	hook->handler = handler;
	hook->irq = irq;
	*line = hook;
	
	irq_enable();
}

void unregister_interrupt_handler(struct irq_hook *hook)
{
	int irq = hook->irq;
	struct irq_hook **line;

	if (irq < 0 || irq >= 256)
		PANIC("unregister_interrupt_handler: invalid irq!\n");
	
	irq_disable();

	line = &_interrupt_handlers[irq];
	while (*line) {
		if ((*line) == hook) {
			*line = (*line)->next;
			break;
		}
		
		line = &((*line)->next);
	}
	
	irq_enable();
}
