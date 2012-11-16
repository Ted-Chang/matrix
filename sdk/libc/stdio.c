/*
 * stdio.c
 */
#include <types.h>
#include <limit.h>
#include <ctype.h>
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include "div64.h"
#ifdef __KERNEL__
#include "mm/page.h"		// PAGE_SIZE
#else
#define PAGE_SIZE	4096
#endif	/* __KERNEL__ */

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


char _tbuf[32];
char _digits[] = "0123456789ABCDEF";

/*
 * Convert a numeric value to a unsigned string
 */
void itoa(unsigned i, unsigned base, char *buf)
{
	int pos = 0;
	int opos = 0;
	int top = 0;

	if (i == 0 || base > 16) {
		buf[0] = '0';
		buf[1] = '\0';
		return;
	}
	
	while (i != 0) {
		_tbuf[pos] = _digits[i % base];
		pos++;
		i /= base;
	}
	top = pos--;
	for (opos = 0; opos < top; pos--, opos++) {
		buf[opos] = _tbuf[pos];
	}
	buf[opos] = 0;
}

/*
 * Convert a numeric value to a signed string
 */
void itoa_s(int i, unsigned base, char *buf)
{
	if (base > 16) return;
	if (i < 0) {
		*buf++ = '-';
		i *= -1;
	}
	itoa(i, base, buf);
}

long strtol(const char *str, char **endstr, int radix)
{
	int c;
	const char *s = str;
	unsigned long acc;
	unsigned long cutoff;
	int neg = 0, any, cutlim;

	/*
	 * Skip white space and pick up leading +/- sign if any.
	 * If radix is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if radix is already 16, allow 0x.
	 */
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	if ((radix == 0 || radix == 16) &&
		c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		radix = 16;
	} else if ((radix == 0 || radix == 2) && 
		c == '0' && (*s == 'b' || *s == 'B')) {
		c = s[1];
		s += 2;
		radix = 2;
	}
	if (radix == 0)
		radix = (c == '0' ? 8 : 10);

	/*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers. That is the largest legal value, divided by the 
	 * radix. An input number that is greater than this value, if 
	 * followed by a legal input character, is too big. One that
	 * is equal to this value may be valid or not; The limit between
	 * valid and invalid numbers is then based on the last digit.
	 * For instance, if the range for longs is [-2147483648..2147483647]
	 * and the input radix is 10, cutoff will be set to 214748364
	 * and cutlim to either 7 (neg == 0) or 8 (neg == 1), meaning
	 * that if we have accumulated a value > 214748364, or equal
	 * but the next digit is > 7 (or 8), the number is too big, 
	 * and we will return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	cutoff = neg ? -(unsigned long)LONG_MIN : LONG_MAX;
	cutlim = cutoff % (unsigned long)radix;
	cutoff /= (unsigned long)radix;

	for (acc = 0, any = 0; ; c = *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= radix)
			break;
		if ((any < 0) || (acc > cutoff) || ((acc == cutoff) && (c > cutlim)))
			any = -1;
		else {
			any = 1;
			acc *= radix;
			acc += c;
		}
	}
	if (any < 0) {
		acc = neg ? LONG_MIN : LONG_MAX;
	} else if (neg)
		acc = -acc;
	if (endstr != NULL)
		*endstr = (char *)(any ? s - 1 : str);
	return (long)acc;
}

/* Converts a string to an unsigned long */
unsigned long strtoul(const char *str, char ** endstr, int radix)
{
	int c;
	const char *s = str;
	unsigned long acc;
	unsigned long cutoff;
	int neg = 0, any, cutlim;

	/*
	 * See strtol for comments as to the logic used.
	 */
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	if ((radix == 0 || radix == 16) && 
		c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		radix = 16;
	} else if ((radix == 0 || radix == 2) && 
		c == '0' && (*s == 'b' || *s == 'B')) {
		c = s[1];
		s += 2;
		radix = 2;
	}
	if (radix == 0)
		radix = (c == '0' ? 8 : 10);
	cutoff = (unsigned long)ULONG_MAX / (unsigned long)radix;
	cutlim = (unsigned long)ULONG_MAX % (unsigned long)radix;
	for (acc = 0, any = 0; ; c = *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= (isupper(c) ? 'A' - 10 : 'a' - 10);
		else 
			break;
		if (c >= radix)
			break;
		if ((any < 0) || (acc > cutoff) || ((acc == cutoff) && (c > cutlim)))
			any = -1;
		else {
			any = 1;
			acc *= radix;
			acc += c;
		}
	}
	if (any < 0) {
		acc = ULONG_MAX;
	} else if (neg)
		acc = -acc;
	if (endstr != 0)
		*endstr = (char *)(any ? s - 1 : str);
	return acc;
}

int atoi(const char *str)
{
	return (int)strtol(str, 0, 10);
}

static int skip_atoi(const char **s)
{
	int i = 0;

	while (isdigit(**s)) {
		i = i*10 + *((*s)++) - '0';
	}

	return i;
}

static char *put_dec_trunc(char *buf, unsigned q)
{
	unsigned char d3, d2, d1, d0;
	d1 = (q>>4) & 0xF;
	d2 = (q>>8) & 0xF;
	d3 = (q>>12);

	d0 = 6*(d3 + d2 + d1) + (q & 0xF);
	q = (d0 * 0xCD) >> 11;
	d0 = d0 - 10*q;		// least significant digit
	*buf++ = d0 + '0';
	d1 = q + 9*d3 + 5*d2 + d1;
	if (d1 != 0) {
		q = (d1 * 0xCD) >> 11;
		d1 = d1 - 10*q;
		*buf++ = d1 + '0';	// next digit

		d2 = q + 2*d2;
		if ((d2 != 0) || (d3 != 0)) {
			q = (d2 * 0xD) >> 7;
			d2 = d2 - 10*q;
			*buf++ = d2 + '0';	// next digit

			d3 = q + 4*d3;
			if (d3 != 0) {
				q = (d3 * 0xCD) >> 11;
				d3 = d3 - 10*q;
				*buf++ = d3 + '0';	// next digit
				if (q != 0)
					*buf++ = q + '0'; // most significant digit
			}
		}
	}

	return buf;
}

static char *put_dec_full(char *buf, unsigned q)
{
	unsigned char d3, d2, d1, d0;
	d1 = (q>>4) & 0xF;
	d2 = (q>>8) & 0xF;
	d3 = (q>>12);

	d0 = 6*(d3 + d2 + d1) + (q & 0xF);
	q = (d0 * 0xCD) >> 11;
	d0 = d0 - 10*q;
	*buf++ = d0 + '0';
	d1 = q + 9*d3 + 5*d2 + d1;
	q = (d1 * 0xCD) >> 11;
	d1 = d1 - 10*q;
	*buf++ = d1 + '0';

	d2 = q + 2*d2;
	q = (d2 * 0xD) >> 7;
	d2 = d2 - 10*q;
	*buf++ = d2 + '0';

	d3 = q + 4*d3;
	q = (d3 * 0xCD) >> 11;

	d3 = d3 - 10*q;
	*buf++ = d3 + '0';
	*buf++ = q + '0';

	return buf;
}

static char *put_dec(char *buf, unsigned long long num)
{
	while (1) {
		unsigned rem;
		if (num < 100000)
			return put_dec_trunc(buf, num);
		rem = do_div(num, 100000);	// NOTE that do_div is a macro here and
						// it will modify the value of num
		buf = put_dec_full(buf, rem);
	}
}

static char *number_helper(printf_helper_t helper, unsigned long long num,
			   struct printf_spec spec)
{
	/* We are called with base 8, 10 or 16 only */
	char c;
	char tmp[66];
	char sign;
	char locase;
	int need_pfx = ((spec.flags & SPECIAL) && spec.base != 10);
	int i;
	
	locase = (spec.flags & SMALL);
	if (spec.flags & LEFT)
		spec.flags &= ~ZEROPAD;
	sign = 0;
	if (spec.flags & SIGN) {
		if ((long long)num < 0) {
			sign = '-';
			num = -(long long)num;
			spec.field_width--;
		} else if (spec.flags & PLUS) {
			sign = '+';
			spec.field_width--;
		} else if (spec.flags & SPACE) {
			sign = ' ';
			spec.field_width--;
		}
	}
	if (need_pfx) {
		spec.field_width--;
		if (spec.base == 16)
			spec.field_width--;
	}

	/* generate full string in tmp[], in reverse order */
	i = 0;
	if (num == 0)
		tmp[i++] = '0';
	else if (spec.base != 10) {	// 8 or 16
		int mask = spec.base - 1;
		int shift = 3;

		if (spec.base == 16)
			shift = 4;
		do {
			tmp[i++] = (_digits[((unsigned char)num) & mask] | locase);
			num >>= shift;
		} while (num);
	} else {			// Base 10
		i = put_dec(tmp, num) - tmp;
	}

	/* printing 100 using %2d gives "100", not "00" */
	if (i > spec.precision)
		spec.precision = i;
	/* leading space padding */
	spec.field_width -= spec.precision;
	if (!(spec.flags & (ZEROPAD+LEFT))) {
		while (--spec.field_width >= 0) {
			c = ' ';
			helper(&c, 1);
		}
	}
	/* sign */
	if (sign) {
		c = sign;
		helper(&c, 1);
	}
	/* "0x" or "0" prefix */
	if (need_pfx) {
		c = '0';
		helper(&c, 1);
		if (spec.base == 16) {
			c = ('X' | locase);
			helper(&c, 1);
		}
	}
	/* zero or space padding */
	if (!(spec.flags & LEFT)) {
		char ch = (spec.flags & ZEROPAD) ? '0' : ' ';
		while (--spec.field_width >= 0) {
			c = ch;
		}
	}
	/* hmm even more zero padding? */
	while (i <= --spec.precision) {
		c = '0';
		helper(&c, 1);
	}
	/* actual digits of result */
	while (--i >= 0) {
		c = tmp[i];
		helper(&c, 1);
	}
	/* trailing space padding */
	while (--spec.field_width >= 0) {
		c = ' ';
		helper(&c, 1);
	}

	return NULL;
}

static char *string_helper(printf_helper_t helper, const char *s, struct printf_spec spec)
{
	char c;
	int len;

	if ((unsigned long)s < PAGE_SIZE)
		s = "(null)";

	len = strnlen(s, spec.precision);

	if (!(spec.flags & LEFT)) {
		while (len < spec.field_width--) {
			c = ' ';
			helper(&c, 1);
		}
	}
	helper(s, len);
	s += len;
	while (len < spec.field_width--) {
		c = ' ';
		helper(&c, 1);
	}

	return NULL;
}

static char *pointer_helper(printf_helper_t helper, const char *fmt, void *ptr,
			    struct printf_spec spec)
{
	if (!ptr && *fmt != 'K') {
		/* Print (null) with the same width as a pointer so it makes
		 * tabular output look nice.
		 */
		if (spec.field_width == -1)
			spec.field_width = 2 * sizeof(void *);
		return string_helper(helper, "(null)", spec);
	}

	spec.flags |= SMALL;
	if (spec.field_width == -1) {
		spec.field_width = 2 * sizeof(void *);
		spec.flags |= ZEROPAD;
	}
	spec.base = 16;

	return number_helper(helper, (unsigned long)ptr, spec);
}

static int format_decode(const char *fmt, struct printf_spec *spec)
{
	const char *start = fmt;

	/* we finished early by reading the field width */
	if (spec->type == FORMAT_TYPE_WIDTH) {
		if (spec->field_width < 0) {
			spec->field_width = -spec->field_width;
			spec->flags |= LEFT;
		}
		spec->type = FORMAT_TYPE_NONE;
		goto precision;
	}

	/* we finished early by reading the precision */
	if (spec->type == FORMAT_TYPE_PRECISION) {
		if (spec->precision < 0)
			spec->precision = 0;
		spec->type = FORMAT_TYPE_NONE;
		goto qualifier;
	}

	/* By default */
	spec->type = FORMAT_TYPE_NONE;

	for (; *fmt; ++fmt) {
		if (*fmt == '%')
			break;
	}

	/* Return the current non-format string */
	if (fmt != start || !*fmt)
		return fmt - start;

	/* Process flags */
	spec->flags = 0;

	while (1) {	// This also skip first `%'
		boolean_t found = TRUE;

		++fmt;

		switch (*fmt) {
		case '-': spec->flags |= LEFT; break;
		case '+': spec->flags |= PLUS; break;
		case ' ': spec->flags |= SPACE; break;
		case '#': spec->flags |= SPECIAL; break;
		case '0': spec->flags |= ZEROPAD; break;
		default: found = FALSE;
		}

		if (!found)
			break;
	}

	/* Get field width */
	spec->field_width = -1;

	if (isdigit(*fmt))
		spec->field_width = skip_atoi(&fmt);
	else if (*fmt == '*') {
		/* It's the next argument */
		spec->type = FORMAT_TYPE_WIDTH;
		return ++fmt - start;
	}

 precision:
	/* get the precision */
	spec->precision = -1;
	if (*fmt == '.') {
		++fmt;
		if (isdigit(*fmt)) {
			spec->precision = skip_atoi(&fmt);
			if (spec->precision < 0)
				spec->precision = 0;
		} else if (*fmt == '*') {
			/* It's the next argument */
			spec->type = FORMAT_TYPE_PRECISION;
			return ++fmt - start;
		}
	}

 qualifier:
	/* get the conversion qualifier */
	spec->qualifier = -1;
	if (*fmt == 'h' || tolower(*fmt) == 'l' ||
	    tolower(*fmt) == 'z' || *fmt == 't') {
		spec->qualifier = *fmt++;
		if (spec->qualifier == *fmt) {
			if (spec->qualifier == 'l') {
				spec->qualifier = 'L';
				++fmt;
			} else if (spec->qualifier == 'h') {
				spec->qualifier = 'H';
				++fmt;
			}
		}
	}

	/* default base */
	spec->base = 10;
	switch (*fmt) {
	case 'c':
		spec->type = FORMAT_TYPE_CHAR;
		return ++fmt - start;
	case 's':
		spec->type = FORMAT_TYPE_STR;
		return ++fmt - start;
	case 'p':
		spec->type = FORMAT_TYPE_PTR;
		return fmt - start;
		/* skip alnum */
	case 'n':
		spec->type = FORMAT_TYPE_NRCHARS;
		return ++fmt - start;
	case '%':
		spec->type = FORMAT_TYPE_PERCENT_CHAR;
		return ++fmt - start;

		/* integer number formats - setup the flags and break */
	case 'o':
		spec->base = 8;
		break;
	case 'x':
		spec->flags |= SMALL;
		/* Fall through */
	case 'X':
		spec->base = 16;
		break;
	case 'd':
	case 'i':
		spec->flags |= SIGN;
		/* Fall through */
	case 'u':
		break;
	default:
		spec->type = FORMAT_TYPE_INVALID;
		return fmt - start;
	}

	if (spec->qualifier == 'L')
		spec->type = FORMAT_TYPE_LONG_LONG;
	else if (spec->qualifier == 'l') {
		if (spec->flags & SIGN)
			spec->type = FORMAT_TYPE_LONG;
		else
			spec->type = FORMAT_TYPE_ULONG;
	} else if (tolower(spec->qualifier) == 'z') {
		spec->type = FORMAT_TYPE_SIZE_T;
	} else if (spec->qualifier == 't') {
		spec->type = FORMAT_TYPE_PTRDIFF;
	} else if (spec->qualifier == 'H') {
		if (spec->flags & SIGN)
			spec->type = FORMAT_TYPE_BYTE;
		else
			spec->type = FORMAT_TYPE_UBYTE;
	} else if (spec->qualifier == 'h') {
		if (spec->flags & SIGN)
			spec->type = FORMAT_TYPE_SHORT;
		else
			spec->type = FORMAT_TYPE_USHORT;
	} else {
		if (spec->flags & SIGN)
			spec->type = FORMAT_TYPE_INT;
		else
			spec->type = FORMAT_TYPE_UINT;
	}

	return ++fmt - start;
}

int do_printf(printf_helper_t helper, const char *fmt, va_list args)
{
	const char *fmt_ptr;
	unsigned long long num;
	struct printf_spec spec = {0};

	fmt_ptr = fmt;

	while (*fmt) {
		char c;
		const char *old_fmt = fmt;
		int read = format_decode(fmt, &spec);

		fmt += read;

		switch (spec.type) {
		case FORMAT_TYPE_NONE: {
			helper(old_fmt, read);
			break;
		}

		case FORMAT_TYPE_WIDTH:
			spec.field_width = va_arg(args, int);
			break;

		case FORMAT_TYPE_PRECISION:
			spec.precision = va_arg(args, int);
			break;

		case FORMAT_TYPE_CHAR: {
			if (!(spec.flags & LEFT)) {
				while (--spec.field_width > 0) {
					c = ' ';
					helper(&c, 1);
				}
			}
			c = (unsigned char)va_arg(args, int);
			helper(&c, 1);
			while (--spec.field_width > 0) {
				c = ' ';
				helper(&c, 1);
			}
			break;
		}
			
		case FORMAT_TYPE_STR:
			string_helper(helper, va_arg(args, char *), spec);
			break;

		case FORMAT_TYPE_PTR:
			pointer_helper(helper, fmt+1, va_arg(args, void *), spec);
			while (isalnum(*fmt))
				fmt++;
			break;

		case FORMAT_TYPE_PERCENT_CHAR:
			c = '%';
			helper(&c, 1);
			break;

		case FORMAT_TYPE_INVALID:
			c = '%';
			helper(&c, 1);
			break;

		case FORMAT_TYPE_NRCHARS: {
			uint8_t qualifier = spec.qualifier;

			if (qualifier == 'l') {
				//long *ip = va_arg(args, long *);
				//*ip = (str - buf);
			} else if (tolower(qualifier) == 'z') {
				//size_t *ip = va_arg(args, size_t *);
				//*ip = (str - buf);
			} else {
				//int *ip = va_arg(args, int *);
				//*ip = (str - buf);
			}
			break;
		}

		default:
			switch (spec.type) {
			case FORMAT_TYPE_LONG_LONG:
				num = va_arg(args, long long);
				break;
			case FORMAT_TYPE_ULONG:
				num = va_arg(args, unsigned long);
				break;
			case FORMAT_TYPE_LONG:
				num = va_arg(args, long);
				break;
			case FORMAT_TYPE_SIZE_T:
				num = va_arg(args, size_t);
				break;
			case FORMAT_TYPE_PTRDIFF:
				num = va_arg(args, ptrdiff_t);
				break;
			case FORMAT_TYPE_UBYTE:
				num = (unsigned char)va_arg(args, int);
				break;
			case FORMAT_TYPE_BYTE:
				num = (char)va_arg(args, int);
				break;
			case FORMAT_TYPE_USHORT:
				num = (unsigned short)va_arg(args, int);
				break;
			case FORMAT_TYPE_SHORT:
				num = (short)va_arg(args, int);
				break;
			case FORMAT_TYPE_INT:
				num = (int)va_arg(args, int);
				break;
			default:
				num = va_arg(args, unsigned int);
			}

			number_helper(helper, num, spec);
		}
	}

	/* The trailing null byte doesn't count towards the total */
	return fmt - fmt_ptr;
}
