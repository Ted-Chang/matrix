#ifndef __RTLUTIL_H__
#define __RTLUTIL_H__

#include <types.h>
#include "matrix/matrix.h"

/* Verify the checksum of specified range */
static INLINE boolean_t verify_chksum(void *start, size_t size)
{
	size_t i;
	uint8_t chksum = 0;
	uint8_t *range = (uint8_t *)start;

	for (i = 0; i < size; i++) {
		chksum += range[i];
	}

	return (chksum == 0);
}

#endif	/* __RTLUTIL_H__ */
