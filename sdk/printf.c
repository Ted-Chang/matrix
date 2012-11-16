#include <types.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <syscall.h>

#define DBG_BUFF_SIZE	1024

int printf(const char *fmt, ...)
{
	char buf[DBG_BUFF_SIZE];
	va_list args;

	if (!fmt) {
		return 0;
	}

	va_start(args, fmt);
	vsnprintf(buf, DBG_BUFF_SIZE, fmt, args);
	mtx_putstr(buf);
	va_end(args);
	
	return 1;
}
