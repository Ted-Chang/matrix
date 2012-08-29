#ifndef __TIME_H__
#define __TIME_H__

/* Seconds to microseconds */
#define SECS2USECS(secs)	((useconds_t)secs * 1000000)

typedef long clock_t;

struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

typedef long time_t;
typedef long suseconds_t;

struct timeval {
	time_t tv_sec;		/* seconds */
	suseconds_t tv_usec;	/* microseconds */
};

struct timezone {
	int tz_minuteswest;	/* minutes west of Greenwich */
	int tz_dsttime;		/* type of DST correction */
};

int get_cmostime(struct tm *t);
useconds_t time_to_unix(uint32_t year, uint32_t mon, uint32_t day,
			uint32_t hour, uint32_t min, uint32_t sec);

#endif	/* __TIME_H__ */
