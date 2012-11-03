#include <types.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

int sprintf(char *buf, const char *fmt, ...)
{
	int ret;
	va_list args;

	if (!fmt) {
		return 0;
	}

	va_start(args, fmt);
	ret = vsprintf(buf, fmt, args);
	va_end(args);
	
	return ret;
}

int snprintf(char *buf, size_t cnt, const char *fmt, ...)
{
	int ret;
	va_list args;

	if (!fmt) {
		return 0;
	}

	va_start(args, fmt);
	ret = vsnprintf(buf, cnt, fmt, args);
	va_end(args);

	return ret;
}
