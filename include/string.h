#ifndef __STRING_H__
#define __STRING_H__

#include <stddef.h>

int strcmp(const char *str1, const char *str2);

int strncmp(const char *str1, const char *str2, size_t num);

char * strcpy(char *dst, const char *src);

char * strncpy(char *dst, const char *src, size_t num);

size_t strlen(const char *str);

char * strchr(const char *str, int ch);

void * memset(void *dst, char val, size_t count);

void * memcpy(void *dst, const void *src, size_t count);


#endif	/* __STRING_H__ */
