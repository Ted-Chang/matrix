#include <types.h>
#include <time.h>
#include "const.h"
#include "isr.h"
#include "hal.h"
#include "timer.h"
#include "task.h"
#include "debug.h"

#define TIMER_FREQ	1193182L	/* clock frequency for timer in PC and AT */

extern struct timer *_active_timers;
extern void tmrs_exptimers(struct timer **list, clock_t now);

int _current_frequency = 0;
uint32_t _lost_ticks = 0;
clock_t _real_time = 0;
clock_t _next_timeout = 0;
static struct irq_hook _clock_hook;

void do_clocktick()
{
	/* Check if a clock timer is expired and call its callback function */
	if (_next_timeout <= _real_time) {
		tmrs_exptimers(&_active_timers, _real_time);
		_next_timeout = _active_timers == NULL ?
			TIMER_NEVER : _active_timers->exp_time;
	}
}

static void clock_callback(struct registers *regs)
{
	uint32_t ticks;

	/* Get number of ticks and update realtime. */
	ticks = _lost_ticks + 1;
	_lost_ticks = 0;
	_real_time += ticks;
	
	/* Check if do_clocktick() must be called. Done for alarms and scheduling.
	 */
	if ((_next_timeout <= _real_time)) {
		do_clocktick();
	}
}

void init_clock()
{
	uint8_t low;
	uint8_t high;
	uint32_t divisor;

	/* Save the frequency */
	_current_frequency = HZ;
	
	/* Register our timer callback first */
	register_interrupt_handler(IRQ0, &_clock_hook, &clock_callback);

	/* The value we send to the PIT is the value to divide. it's input
	 * clock (1193182 Hz) by, to get our required frequency. Important
	 * note that the divisor must be small enough to fit into 16-bits.
	 */
	divisor = TIMER_FREQ / HZ;

	/* Send the command byte */
	outportb(0x43, 0x36);

	/* Divisor has to be sent byte-wise, so split here into upper/lower bytes */
	low = (uint8_t)(divisor & 0xFF);
	high = (uint8_t)((divisor >> 8) & 0xFF);

	outportb(0x40, low);
	outportb(0x40, high);
}

void stop_clock()
{
	outportb(0x43, 0x36);
	outportb(0x40, 0);
	outportb(0x40, 0);
}

clock_t get_uptime()
{
	return _real_time;
}

u_long read_clock()
{
	u_long count;

	/* Read the counter of channel 0 of the 8253A timer. This counter counts
	 * down at a rate of TIMER_FREQ and restarts at TIMER_COUNT - 1 when it
	 * reaches zero. A hardware interrupt (clock tick) occurs when the counter
	 * gets to zero and restarts its cycle.
	 */
	outportb(0x43, 0x00);
	count = inportb(0x40);
	count |= (inportb(0x40) << 8);

	return count;
}
