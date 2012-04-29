#ifndef __CLOCK_H__
#define __CLOCK_H__

/* Seconds to microseconds */
#define SECS2USECS(secs)	((useconds_t)secs * 1000000)

extern useconds_t time_to_unix(uint32_t year, uint32_t mon, uint32_t day,
			       uint32_t hour, uint32_t min, uint32_t sec);
extern useconds_t system_time();
extern boolean_t do_clocktick();
extern void init_clock();

#endif	/* __CLOCK_H__ */
