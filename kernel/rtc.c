#include <types.h>
#include <stddef.h>
#include "hal/hal.h"
#include "hal/spinlock.h"
#include "matrix/debug.h"
#include "time.h"
#include "clock.h"

#define BCD2DEC(n)	(((n >> 4) & 0x0F) * 10 + (n & 0x0F))
#define DEC2BCD(n)	(((n / 10) << 4) | (n % 10))

struct spinlock _rtc_lock = {
	1,
	FALSE,
	"rtc_lock"
};

static uint32_t rdrtc(int addr)
{
	/* Read a single CMOS register value */
	outportb(0x70, addr);
	return inportb(0x71);
}

useconds_t platform_time_from_cmos()
{
	uint32_t year, mon, day, hour, min, sec;
	uint8_t tmp;

	spinlock_acquire(&_rtc_lock);

	sec = rdrtc(0x00);
	min = rdrtc(0x02);
	hour = rdrtc(0x04);
	day = rdrtc(0x07);
	mon = rdrtc(0x08);
	year = rdrtc(0x09);

	/* If RTC is in BCD mode convert to binary (default RTC mode). */
	if ((rdrtc(0x0B) & 0x04) == 0) {
		min = BCD2DEC(min);
		hour = BCD2DEC(hour);
		day = BCD2DEC(day);
		mon = BCD2DEC(mon);
		year = BCD2DEC(year);
		sec = BCD2DEC(sec);
	}

	/* Correct the year, good until 2080 */
	if (year <= 69)
		year += 100;
	year += 1900;

	spinlock_release(&_rtc_lock);

	return time_to_unix(year, mon, day, hour, min, sec);
}
