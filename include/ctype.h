#ifndef __CTYPE_H__
#define __CTYPE_H__

#define isspace(c)	((c) == ' ' || ((c) >= '\t' && (c) <= '\r'))
#define isascii(c)	(((c) & ~0x7F) == 0)
#define isupper(c)	((c) >= 'A' && (c) <= 'Z')
#define islower(c)	((c) >= 'a' && (c) <= 'z')
#define isalpha(c)	(isupper(c) || islower(c))
#define isdigit(c)	((c) >= '0' && (c) <= '9')
#define isxdigit(c)	(isdigit(c) || \
				((c) >= 'A' && (c) <= 'F') || \
				((c) >= 'a' && (c) <= 'f'))
#define isprint(c)	((c) >= ' ' && (c) <= '~')
#define toupper(c)	((c) - 0x20 * (((c) >= 'a') && ((c) <= 'z')))
#define tolower(c)	((c) + 0x20 * (((c) >= 'A') && ((c) <= 'Z')))
#define toascii(c)	((unsigned)(c) & 0x7F)

#endif	/* __CTYPE_H__ */
