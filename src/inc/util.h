#ifndef __UTIL_H__
#define __UTIL_H__

void putch(char ch);

void clear_scr();

void putstr(const char *str);

int kprintf(const char *str, ...);

#define PANIC(msg)	panic(msg, __FILE__, __LINE__)

#define ASSERT(b)	((b) ? (void)0: panic_assert(__FILE__, __LINE__, #b))

#endif	/* __UTIL_H__ */
