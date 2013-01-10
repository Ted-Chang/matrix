#ifndef __SYS_TIME_H__
#define __SYS_TIME_H__

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

typedef unsigned long clock_t;
typedef long time_t;
typedef int64_t suseconds_t;

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

struct timeval {
	time_t tv_sec;		/* seconds */
	suseconds_t tv_usec;	/* microseconds */
};

struct timezone {
	int tz_minuteswest;	/* minutes west of Greenwich */
	int tz_dsttime;		/* type of DST correction */
};

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif	/* __SYS_TIME_H__ */
