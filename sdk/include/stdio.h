#ifndef __STDIO_H__
#define __STDIO_H__

#include <types.h>
#include <stddef.h>
#include <stdarg.h>

/* printf helper function */
typedef void (*printf_helper_t)(const char *, size_t);

extern long strtol(const char *str, char **endstr, int radix);
extern unsigned long strtoul(const char *str, char **endstr, int radix);
extern int atoi(const char *str);
extern void itoa_s(int i, unsigned base, char *buf);
extern void itoa(unsigned i, unsigned base, char *buf);
extern int sprintf(char *buf, const char *fmt, ...);
extern int snprintf(char *buf, size_t cnt, const char *fmt, ...);
extern int do_printf(printf_helper_t helper, const char *fmt, va_list args);

#ifndef __KERNEL__
extern int printf(const char *fmt, ...);
#endif	/* No __KERNEL__ */

#endif	/* __STDIO_H__ */
