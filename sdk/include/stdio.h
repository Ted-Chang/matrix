#ifndef __STDIO_H__
#define __STDIO_H__

#include <types.h>
#include <stddef.h>

extern long strtol(const char *str, char **endstr, int radix);
extern unsigned long strtoul(const char *str, char **endstr, int radix);
extern int atoi(const char *str);
extern void itoa_s(int i, unsigned base, char *buf);
extern void itoa(unsigned i, unsigned base, char *buf);
extern int sprintf(char *buf, const char *fmt, ...);
extern int snprintf(char *buf, size_t cnt, const char *fmt, ...);

#endif	/* __STDIO_H__ */
