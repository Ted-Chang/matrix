#ifndef __STRING_H__
#define __STRING_H__

#include <stddef.h>

extern int strcmp(const char *str1, const char *str2);
extern int strncmp(const char *str1, const char *str2, size_t num);
extern char *strcpy(char *dst, const char *src);
extern char *strncpy(char *dst, const char *src, size_t num);
extern char *strcat(char *dst, const char *src);
extern char *strncat(char *dst, const char *src, size_t num);
extern size_t strlen(const char *str);
extern size_t strnlen(const char *str, size_t num);
extern char *strchr(const char *str, int ch);
extern void *memset(void *dst, char val, size_t count);
extern void *memcpy(void *dst, const void *src, size_t count);
extern int memcmp(const void *s1, const void *s2, size_t count);

#endif	/* __STRING_H__ */
