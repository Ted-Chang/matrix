#ifndef __STDIO_H__
#define __STDIO_H__

#ifndef NULL
#define NULL	0
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

long strtol(const char *str, char **endstr, int radix);

unsigned long strtoul(const char *str, char **endstr, int radix);

int atoi(const char *str);

#endif	/* __STDIO_H__ */
