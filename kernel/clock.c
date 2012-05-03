#include <types.h>
#include <stddef.h>
#include <time.h>
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "list.h"
#include "hal/isr.h"
#include "hal/hal.h"
#include "pit.h"
#include "tsc.h"
#include "cpu.h"
#include "clock.h"
#include "div64.h"

#define LEAPYEAR(y)	(((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))
#define DAYS(y)		(LEAPYEAR(y) ? 366 : 365)

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

/* The number of microseconds since the Epoch the kernel was booted at */
static useconds_t _boot_time;

/* Hardware timer devices */
struct timer_dev *_timer_dev = NULL;

useconds_t sys_time()
{
	uint64_t value;
	
	ASSERT(CURR_CPU->arch.cycles_per_us != 0);

	value = x86_rdtsc() - CURR_CPU->arch.sys_time_offset;
	do_div(value, CURR_CPU->arch.cycles_per_us);
	
	return (useconds_t)value;
}

void init_tsc_target()
{
	if (CURR_CPU == &_boot_cpu)
		CURR_CPU->arch.sys_time_offset = (useconds_t)x86_rdtsc();
}

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

	/* If this is a leap year, and we've past February, we need to add
	 * another day.
	 */
	if (mon > 2 && LEAPYEAR(year))
		seconds += 24 * 60 * 60;

	/* Add the days in each year before this year from 1970 */
	for (i = 1970; i < year; i++)
		seconds += DAYS(i) * 24 * 60 * 60;

	return SECS2USECS(seconds);
}

void timer_dev_prepare(struct timer *t)
{
	useconds_t len = t->target - sys_time();

	_timer_dev->prepare((len > 0) ? len : 1);
}

boolean_t do_clocktick()
{
	useconds_t time;
	struct list *iter, *n;
	boolean_t preempt;
	struct timer *t;

	ASSERT(_timer_dev);
	
	if (!CURR_CPU->timer_enabled)
		return FALSE;

	time = sys_time();
	preempt = FALSE;
	
	spinlock_acquire(&CURR_CPU->timer_lock);
	
	/* Check if a clock timer on the current CPU is expired and call its
	 * callback function
	 */
	LIST_FOR_EACH_SAFE(iter, n, &CURR_CPU->timers) {
		/* Since the timer list is ordered we break if the current timer
		 * has not expired.
		 */
		t = LIST_ENTRY(iter, struct timer, header);
		if (time < t->target)
			break;

		/* Remove the timer from list if it has expired */
		list_del(&t->header);
	}

	switch (_timer_dev->type) {
	case TIMER_DEV_ONESHOT:
		/* Prepare for the next tick if there is still a timer in the list */
		iter = &CURR_CPU->timers;
		if (!list_empty(iter)) {
			t = LIST_ENTRY(iter->next, struct timer, header);
			timer_dev_prepare(t);
		}
		break;
	case TIMER_DEV_PERIODIC:
		break;
	}
	
	spinlock_release(&CURR_CPU->timer_lock);

	return preempt;
}

void set_timer_dev(struct timer_dev *dev)
{
	ASSERT(!_timer_dev);

	_timer_dev = dev;
	if (_timer_dev->type == TIMER_DEV_ONESHOT) {
		CURR_CPU->timer_enabled = TRUE;
	}

	DEBUG(DL_DBG, ("activated timer device: %s\n", dev->name));
}

void init_clock()
{
	/* Initialize the boot time */
	_boot_time = platform_time_from_cmos() - sys_time();
	DEBUG(DL_DBG, ("Boot time: %ld microseconds\n", _boot_time));
}
