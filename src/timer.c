#include "types.h"
#include "isr.h"
#include "hal.h"
#include "timer.h"
#include "task.h"
#include "debug.h"

uint32_t tick = 0;

static void timer_callback(struct registers regs)
{
	tick++;
	switch_task();
}

void init_timer(uint32_t frequency)
{
	uint32_t divisor;
	uint8_t low;
	uint8_t high;
	
	/* Register our timer callback first */
	register_interrupt_handler(IRQ0, &timer_callback);

	/* The value we send to the PIT is the value to divide. it's input
	 * clock (1193180 Hz) by, to get our required frequency. Important
	 * note that the divisor must be small enough to fit into 16-bits.
	 */
	divisor = 1193180 / frequency;

	/* Send the command byte */
	outportb(0x43, 0x36);

	/* Divisor has to be sent byte-wise, so split here into upper/lower bytes */
	low = (uint8_t)(divisor & 0xFF);
	high = (uint8_t)((divisor >> 8) & 0xFF);

	outportb(0x40, low);
	outportb(0x40, high);
}
