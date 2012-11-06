#ifndef __CLOCK_H__
#define __CLOCK_H__

/* Seconds to microseconds */
#define SECS2USECS(secs)	((useconds_t)secs * 1000000)

extern int get_cmostime(struct tm *t);
extern useconds_t time_to_unix(uint32_t year, uint32_t mon, uint32_t day,
			       uint32_t hour, uint32_t min, uint32_t sec);
extern useconds_t sys_time();
extern void tsc_init_target();
extern useconds_t platform_time_from_cmos();

#endif	/* __CLOCK_H__ */
