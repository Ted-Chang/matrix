#ifndef __STDIO_H__
#define __STDIO_H__

extern long strtol(const char *str, char **endstr, int radix);
extern unsigned long strtoul(const char *str, char **endstr, int radix);
extern int atoi(const char *str);
extern void itoa_s(int i, unsigned base, char *buf);
extern void itoa(unsigned i, unsigned base, char *buf);

#endif	/* __STDIO_H__ */
