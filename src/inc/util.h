#ifndef __UTIL_H__
#define __UTIL_H__

void putch(char ch);

void clear_scr();

void putstr(char *str);

int kprintf(const char *str, ...);

#define PANIC(msg)	panic(msg, __FILE__, __LINE__)

#endif	/* __UTIL_H__ */
