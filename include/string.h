#ifndef __STRING_H__
#define __STRING_H__

#include <size_t.h>

int strcmp(const char *str1, const char *str2);

char * strcpy(char *dst, const char *src);

size_t strlen(const char *str);

char * strchr(const char *str, int ch);

void * memset(void *dst, char val, size_t count);

void * memcpy(void *dst, const void *src, size_t count);


#endif	/* __STRING_H__ */
