#include <types.h>
#include <stddef.h>
#include <sys/time.h>
#include "matrix/matrix.h"
#include "matrix/const.h"
#include "matrix/global.h"
#include "isr.h"
#include "hal.h"
#include "timer.h"
#include "proc/task.h"
#include "proc/sched.h"
#include "matrix/debug.h"

#define LEAPYEAR(y)	(((y) % 4) == 0 && ((y) % 100) != 0 || (((y) % 400) == 0))
#define DAYS(y)		(LEAPYEAR(y) ? 366 : 365)

#define TIMER_FREQ	1193182L	// clock frequency for timer in PC and AT

extern struct timer *_active_timers;
extern void tmrs_exptimers(struct timer **list, clock_t now);

int _current_frequency = 0;
uint32_t _lost_ticks = 0;
clock_t _real_time = 0;
clock_t _next_timeout = 0;
static struct irq_hook _clock_hook;

/* Table containing number of days before a month */
static int _days_before_month[] = {
	0,
	0,
	31,
	31 + 28,
	31 + 28 + 31,
	31 + 28 + 31 + 30,
	31 + 28 + 31 + 30 + 31,
	31 + 28 + 31 + 30 + 31 + 30,
	31 + 28 + 31 + 30 + 31 + 30 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
};

useconds_t time_to_unix(uint32_t year, uint32_t mon, uint32_t day,
			uint32_t hour, uint32_t min, uint32_t sec)
{
	uint32_t seconds = 0;
	uint32_t i;

	seconds += sec;
	seconds += min * 60;
	seconds += hour * 60 * 60;
	seconds += (day - 1) * 24 * 60 * 60;

	/* Convert the month into seconds */
	seconds += _days_before_month[mon] * 24 * 60 * 60;

	/* If this is a leap year and we've past February, we need to
	 * add another day.
	 */
	if (mon > 2 && LEAPYEAR(year))
		seconds += 24 * 60 * 60;

	/* Add the days in each year before this year from 1970 */
	for (i = 1970; i < year; i++)
		seconds += DAYS(i) * 24 * 60 * 60;

	return SECS2USECS(seconds);
}

void do_clocktick()
{
	/* We will not switch task if the task didn't use up a full quantum. */
	if ((_prev_task->ticks_left <= 0) &&
	    (FLAG_ON(_prev_task->priv.flags, PREEMPTIBLE))) {

		sched_dequeue(_prev_task);
		sched_enqueue(_prev_task);
		
		/* Task scheduling was done, then do context switch */
		switch_context();
	}

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

	/* If multitask was not initialized, just return */
	if (!CURR_PROC) return;

	/* Update user and system accounting times. Charge the current process for
	 * user time. If the current process is not billable, that is, if a non-user
	 * process is running, charge the billable process for system time as well.
	 * Thus the unbillable process' user time is the billable user's system time.
	 */
	CURR_PROC->usr_time += ticks;
	if (FLAG_ON(CURR_PROC->priv.flags, PREEMPTIBLE)) {
		CURR_PROC->ticks_left -= ticks;	// Consume the quantum
	}
	
	/* Check if do_clocktick() must be called. Done for alarms and scheduling.
	 */
	if ((_next_timeout <= _real_time) || (CURR_PROC->ticks_left <= 0)) {
		_prev_task = CURR_PROC;		// Store running task
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
