#include <types.h>
#include <stddef.h>
#include <ctype.h>
#include "intrnl.h"
#include "div64.h"

int skip_atoi(const char **s)
{
	int i = 0;

	while (isdigit(**s))
		i = i*10 + *((*s)++) - '0';

	return i;
}

char *put_dec_trunc(char *buf, unsigned q)
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

char *put_dec_full(char *buf, unsigned q)
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

char *put_dec(char *buf, unsigned long long num)
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

int format_decode(const char *fmt, struct printf_spec *spec)
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
