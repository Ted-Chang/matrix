#include <types.h>
#include <stddef.h>
#include "sys/time.h"
#include "timer.h"
#include "clock.h"

struct timer tmr;

static void timer_callback(struct timer *tp)
{
	struct tm date_time;

	get_cmostime(&date_time);

	kprintf("year:%d, month:%d, day:%d, hour:%d, minute:%d, second:%d\n",
		date_time.tm_year, date_time.tm_mon, date_time.tm_mday,
		date_time.tm_hour, date_time.tm_min, date_time.tm_sec);

	/* Reschedule the timer again */
	set_timer(&tmr, 1000, timer_callback);
}

void timer_test()
{
	set_timer(&tmr, 1000, timer_callback);
}
