#include <types.h>
#include <stddef.h>
#include "sys/time.h"
#include "matrix/matrix.h"
#include "matrix/const.h"
#include "hal/isr.h"
#include "hal/hal.h"
#include "hal/core.h"
#include "timer.h"
#include "proc/process.h"
#include "proc/sched.h"
#include "debug.h"
#include "div64.h"
#include "pit.h"

#define LEAPYEAR(y)	(((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))
#define DAYS(y)		(LEAPYEAR(y) ? 366 : 365)

static struct irq_hook _pit_hook;

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

/* Boot CORE system time value */
static useconds_t _sys_time_sync = 0;

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
	if (mon > 2 && LEAPYEAR(year)) {
		seconds += 24 * 60 * 60;
	}

	/* Add the days in each year before this year from 1970 */
	for (i = 1970; i < year; i++) {
		seconds += DAYS(i) * 24 * 60 * 60;
	}

	return SECS2USECS(seconds);
}

useconds_t sys_time()
{
	useconds_t value;

	value = (x86_rdtsc() - CURR_CORE->arch.sys_time_offset);
	
	do_div(value, CURR_CORE->arch.cycles_per_us);

	return value;
}

static void pit_callback(struct registers *regs)
{
	timer_tick();
}

void tsc_init_target()
{
	/* Calculate the offset to subtract from the TSC when calculating the
	 * system time. For the boot CORE, this is the current value of the TSC.
	 */
	if (CURR_CORE == &_boot_core) {
		CURR_CORE->arch.sys_time_offset = x86_rdtsc();
	} else {
		/* Tell the boot CORE that we are in phrase TSC_SYNC1 */
		//...
		
		/* Calculate the offset we need to use */
		CURR_CORE->arch.sys_time_offset =
			-((_sys_time_sync * CURR_CORE->arch.cycles_per_us) -
			  x86_rdtsc());
	}
}

void tsc_init_source()
{
	/* Wait for the AC to get into tsc_init_target() */
	//...

	/* Save our sys_time() value */
	_sys_time_sync = sys_time();
}

void init_pit()
{
	uint8_t low;
	uint8_t high;
	uint32_t divisor;

	/* Register our timer callback first */
	register_irq_handler(IRQ0, &_pit_hook, &pit_callback);

	/* The value we send to the PIT is the value to divide. it's input
	 * clock (1193182 Hz) by, to get our required frequency. Important
	 * note that the divisor must be small enough to fit into 16-bits.
	 */
	divisor = PIT_BASE_FREQ / HZ;

	/* Send the command byte */
	outportb(0x43, 0x36);

	/* Divisor has to be sent byte-wise, so split here into upper/lower bytes */
	low = (uint8_t)(divisor & 0xFF);
	high = (uint8_t)((divisor >> 8) & 0xFF);

	outportb(0x40, low);
	outportb(0x40, high);
}

void stop_pit()
{
	outportb(0x43, 0x36);
	outportb(0x40, 0);
	outportb(0x40, 0);
}

void spin(useconds_t us)
{
	uint64_t tgt = x86_rdtsc() + (us * CURR_CORE->arch.cycles_per_us);

	/* Spin until we reach the target */
	while (x86_rdtsc() < tgt) {
		core_spin_hint();
	}
}
