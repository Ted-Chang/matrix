#include <types.h>
#include <stddef.h>
#include <string.h>
#include "mm/malloc.h"
#include "kstrdup.h"

char *kstrdup(const char *s, int flags)
{
	char *str;
	size_t len;

	len = strlen(s);
	
	str = kmalloc(len + 1, flags);
	if (str) {
		strcpy(str, s);
	}

	return str;
}

char *kstrndup(const char *s, size_t n, int flags)
{
	char *str;
	size_t len;

	len = strlen(s);

	if (len > n) {
		len = n;
	}

	str = kmalloc(len + 1, flags);
	if (str) {
		strncpy(str, s, n);
		str[len] = '\0';
	}

	return str;
}
