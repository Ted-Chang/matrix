#ifndef __BITOPS_H__
#define __BITOPS_H__

#include <types.h>

static INLINE int bitops_ffs(u_long value)
{
	asm ("bsf %1, %0" : "=r"(value) : "rm"(value) : "cc");
	return (int)value;
}

static INLINE int bitops_ffz(u_long value)
{
	asm ("bsf %1, %0" : "=r"(value) : "r"(~value) : "cc");
	return (int)value;
}

static INLINE int bitops_fls(u_long value)
{
	asm ("bsr %1, %0" : "=r"(value) : "rm"(value) : "cc");
	return (int)value;
}

#endif	/* __BITOPS_H__ */
