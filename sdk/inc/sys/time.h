#ifndef __TIME_H__
#define __TIME_H__

typedef unsigned long clock_t;

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

#endif	/* __TIME_H__ */
