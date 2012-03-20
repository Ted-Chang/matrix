#include "types.h"
#include "isr.h"
#include "hal.h"
#include "timer.h"
#include "task.h"
#include "debug.h"

int _current_frequency = 0;
uint32_t _tick = 0;
static struct irq_hook _timer_hook;

extern void switch_task();

static void timer_callback(struct registers *regs)
{
	/* Increase the tick count */
	_tick++;

	/* Do task switch */
	switch_task();
}

void init_timer(uint32_t frequency)
{
	uint8_t low;
	uint8_t high;
	uint32_t divisor;

	/* Save the frequency */
	_current_frequency = frequency;
	
	/* Register our timer callback first */
	register_interrupt_handler(IRQ0, &_timer_hook, &timer_callback);

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
