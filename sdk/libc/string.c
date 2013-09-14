/*
 * string.c
 */
#include <types.h>
#include <stdio.h>
#include <string.h>

int strcmp(const char *str1, const char *str2)
{
	int res = 0;
	while (!(res = *(unsigned char *)str1 - *(unsigned char *)str2) && *str2)
		++str1, ++str2;

	if (res < 0)
		res = -1;
	if (res > 0)
		res = 1;

	return res;
}

int strncmp(const char *str1, const char *str2, size_t num)
{
	int res = 0;
	while (num-- && !(res = *(unsigned char *)str1 - *(unsigned char *)str2) && *str2)
		++str1, ++str2;
	if (res < 0)
		res = -1;
	if (res > 0)
		res = 1;
	
	return res;
}

char *strcpy(char *dst, const char *src)
{
	char *cp = dst;
	while ((*(cp++) = *(src++)));
	return (dst);
}

char *strncpy(char *dst, const char *src, size_t num)
{
	char *cp = dst;
	while (num-- && (*(cp++) = *(src++)));
	return dst;
}

char *strcat(char *dst, const char *src)
{
	char *cp = dst;
	while (*cp) cp++;		// Find the end of dst
	while ((*(cp++) = *(src++)));	// Copy src to end of dst
	return dst;
}

char *strncat(char *dst, const char *src, size_t num)
{
	char *cp = dst;
	while(*cp) cp++;		// Find the end of dst
	while(num-- && (*(cp++) = *(src++)));
	return dst;
}

size_t strlen(const char *str)
{
	size_t len = 0;
	while (str[len++]);
	return len - 1;
}

size_t strnlen(const char *str, size_t num)
{
	size_t len = 0;
	while (str[len++] && (len <= num));
	return len - 1;
}

char *strchr(const char *str, int ch)
{
	while (*str && *str != (char)ch) str++;
	if (*str == (char)ch)
		return ((char*)str);
	return NULL;
}

void *memset(void *dst, char val, size_t count)
{
	unsigned char *temp = (unsigned char *)dst;
	for (; count != 0; count--, temp[count] = val);
	return dst;
}

void *memcpy(void *dst, const void *src, size_t count)
{
	const char *sp = (const char *)src;
	char *dp = (char *)dst;
	for (; count != 0; count--)
		*dp++ = *sp++;
	return dst;
}

int memcmp(const void *s1, const void *s2, size_t count)
{
	int res = 0;
	uint8_t *ptr1 = (uint8_t *)s1;
	uint8_t *ptr2 = (uint8_t *)s2;
	
	while (count-- && !(res = *(unsigned char *)ptr1 - *(unsigned char *)ptr2))
		++ptr1, ++ptr2;
	if (res < 0)
		res = -1;
	if (res > 0)
		res = 1;
	
	return res;
}
