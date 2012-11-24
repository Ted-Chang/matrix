#ifndef __INTRNL_H__
#define __INTRNL_H__

#define ZEROPAD	1		// pad with zero
#define SIGN	2		// unsigned or signed long
#define PLUS	4		// show plus
#define SPACE	8		// space if plus
#define LEFT	16		// left justified
#define SMALL	32		// use lowercase in hex
#define SPECIAL	64		// prefix hex with "0x", octal with "0"

enum format_type {
	FORMAT_TYPE_NONE, /* Just a string part */
	FORMAT_TYPE_WIDTH,
	FORMAT_TYPE_PRECISION,
	FORMAT_TYPE_CHAR,
	FORMAT_TYPE_STR,
	FORMAT_TYPE_PTR,
	FORMAT_TYPE_PERCENT_CHAR,
	FORMAT_TYPE_INVALID,
	FORMAT_TYPE_LONG_LONG,
	FORMAT_TYPE_ULONG,
	FORMAT_TYPE_LONG,
	FORMAT_TYPE_UBYTE,
	FORMAT_TYPE_BYTE,
	FORMAT_TYPE_USHORT,
	FORMAT_TYPE_SHORT,
	FORMAT_TYPE_UINT,
	FORMAT_TYPE_INT,
	FORMAT_TYPE_NRCHARS,
	FORMAT_TYPE_SIZE_T,
	FORMAT_TYPE_PTRDIFF
};

struct printf_spec {
	uint8_t type;		// format type enum
	uint8_t flags;		// flags to number()
	uint8_t base;		// number base 8, 10 or 16 only
	uint8_t qualifier;	// number qualifier, one of `hHlLtzZ'
	int16_t field_width;	// width of output field
	int16_t precision;	// # of digits/chars
};

int skip_atoi(const char **s);
char *put_dec_trunc(char *buf, unsigned q);
char *put_dec_full(char *buf, unsigned q);
char *put_dec(char *buf, unsigned long long num);
int format_decode(const char *fmt, struct printf_spec *spec);

#endif	/* __INTRNL_H__ */
