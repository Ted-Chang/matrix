#ifndef __STDIO_H__
#define __STDIO_H__

long strtol(const char *str, char **endstr, int radix);

unsigned long strtoul(const char *str, char **endstr, int radix);

int atoi(const char *str);

void itoa_s(int i, unsigned base, char *buf);

void itoa(unsigned i, unsigned base, char *buf);

#endif	/* __STDIO_H__ */
