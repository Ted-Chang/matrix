/*
 * stdio.c
 */
#include <types.h>
#include <limit.h>
#include <ctype.h>
#include <stdio.h>


char _tbuf[32];
char _bchars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

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
		_tbuf[pos] = _bchars[i % base];
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
