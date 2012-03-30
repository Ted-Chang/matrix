#include <types.h>
#include <time.h>
#include "matrix/matrix.h"
#include "cmos.h"
#include "hal.h"

#define BCD2DEC(n)	(((n >> 4) & 0x0F) * 10 + (n & 0x0F))
#define DEC2BCD(n)	(((n / 10) << 4) | (n % 10))

static int read_register(int reg_addr)
{
	/* Read a single CMOS register value */
	outportb(RTC_INDEX, reg_addr);
	return inportb(RTC_IO);
}

int get_cmostime(struct tm *t)
{
	/* TODO: Start a timer to keep us from getting stuck on a dead clock. */
	t->tm_sec = read_register(RTC_SEC);
	t->tm_min = read_register(RTC_MIN);
	t->tm_hour = read_register(RTC_HOUR);
	t->tm_mday = read_register(RTC_MDAY);
	t->tm_mon = read_register(RTC_MONTH);
	t->tm_year = read_register(RTC_YEAR);
	
	if ((read_register(RTC_REG_B) & RTC_B_DM_BCD) == 0) {
		/* Convert BCD to binary (default RTC mode). */
		t->tm_min = BCD2DEC(t->tm_min);
		t->tm_hour = BCD2DEC(t->tm_hour);
		t->tm_mday = BCD2DEC(t->tm_mday);
		t->tm_mon = BCD2DEC(t->tm_mon);
		t->tm_year = BCD2DEC(t->tm_year);
		t->tm_sec = BCD2DEC(t->tm_sec);
	}

	/* Correct the year, good until 2080 */
	if (t->tm_year <= 69)
		t->tm_year += 100;
	t->tm_year += 1900;

	return 0;
}

