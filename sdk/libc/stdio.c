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
#include "format.h"
#include "div64.h"
#ifdef __KERNEL__
#include "mm/page.h"		// PAGE_SIZE
#else
#define PAGE_SIZE	4096
#endif	/* __KERNEL__ */

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
