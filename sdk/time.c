#include <types.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/time.h>

#define PUTCH(b, m, t, c) \
	if ((t) < (m)) { \
		(b)[(t)] = c;	\
	} \
	(t)++

#define PUTSTR(b, m, t, fmt...) \
	__extension__ \
	({ \
		int __count = ((int)(m) + 1) - (int)(t);	\
		if (__count < 0) { \
			__count = 0;		\
		} \
		snprintf(&(b)[(t)], __count, fmt); \
	})

static const char *_mon_abbrev[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
	"Nov", "Dec",
};

static const char *_day_abbrev[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
};

static const char *_am_pm[] = {
	"AM", "am", "PM", "pm",
};

time_t time(time_t *tp)
{
	struct timeval tv;

	if (0 != gettimeofday(&tv, NULL)) {
		return -1;
	}

	if (tp) {
		*tp = tv.tv_sec;
	}

	return tv.tv_sec;
}

size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tm)
{
	boolean_t modif_e, modif_o;
	size_t total = 0;
	int state = 0;
	char ch;

	if (max == 0) {
		return 0;
	}

	/* Reserve space for the null terminator */
	max -= 1;

	while (*fmt) {
		ch = *(fmt++);

		switch (state) {
		case 0:
			/* Wait for % */
			if (ch == '%') {
				state = 1;
			} else {
				PUTCH(s, max, total, ch);
			}
			break;
		case 1:
			/* Wait for modifiers */
			modif_e = modif_o = FALSE;
			state = 2;

			/* Check for literal % and modifiers */
			switch (ch) {
			case '%':
				/* Literal % */
				PUTCH(s, max, total, ch);
				state = 0;
				break;
			case 'E':
				modif_e = TRUE;
				break;
			case 'O':
				modif_o = TRUE;
				break;
			}

			if (state == 0 || modif_e || modif_o) {
				break;
			}

			/* Fall through */
		case 2:
			/* Handle conversion specifiers */
			switch (ch) {
			case 'a':
				total += PUTSTR(s, max, total, "%s", _day_abbrev[tm->tm_wday]);
				break;
			case 'A':
				break;
			case 'b':
			case 'h':
				total += PUTSTR(s, max, total, "%s", _mon_abbrev[tm->tm_mon]);
				break;
			case 'B':
				break;
			case 'd':
				total += PUTSTR(s, max, total, "%02d", tm->tm_mday);
				break;
			case 'D':
				total += PUTSTR(s, max, total, "%02d/%02d/%02d", tm->tm_mon + 1, tm->tm_mday, tm->tm_year % 100);
				break;
			case 'e':
				break;
			case 'F':
				break;
			case 'H':
				break;
			case 'I':
				break;
			case 'j':
				break;
			case 'n':
				PUTCH(s, max, total, '\n');
				break;
			case 'm':
				break;
			case 'M':
				total += PUTSTR(s, max, total, "%02d", tm->tm_min);
				break;
			case 'p':
				total += PUTSTR(s, max, total, "%s", _am_pm[tm->tm_hour > 11 ? 2 : 0]);
				break;
			case 'P':
				total += PUTSTR(s, max, total, "%s", _am_pm[tm->tm_hour > 11 ? 3 : 1]);
				break;
			case 'S':
				total += PUTSTR(s, max, total, "%02d", tm->tm_sec);
				break;
			case 'y':
				total += PUTSTR(s, max, total, "%02d", tm->tm_year % 100);
				break;
			case 'Y':
				total += PUTSTR(s, max, total, "%d", tm->tm_year + 1900);
				break;
			case 'z':
				total += PUTSTR(s, max, total, "+0000");
				break;
			case 'Z':
				total += PUTSTR(s, max, total, "UTC");
				break;
			}
			state = 0;
			break;
		}
	}

	s[total] = 0;
	return (total <= max) ? total : 0;
}
