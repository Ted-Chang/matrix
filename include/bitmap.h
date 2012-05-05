#ifndef __BITMAP_H__
#define __BITMAP_H__

#include "matrix/matrix.h"
#include "mm/mm.h"

/* Bitmap structure definition */
struct bitmap {
	uint8_t *data;		// Bitmap data
	int nr_bits;		// Number of bits in the bitmap
	boolean_t allocated;	// Whether data was allocated by bitmap_init()
};

/* Get the number of bytes required for a bitmap */
#define BITMAP_BYTES(bits)	(ROUND_UP(bits, 8) / 8)

static INLINE int bitops_ffs(unsigned long value)
{
	asm volatile("bsf %1, %0" : "=r"(value) : "rm"(value) : "cc");
	return (int)value;
}

static INLINE int bitops_ffz(unsigned long value)
{
	asm volatile("bsf %1, %0" : "=r"(value) : "r"(~value) : "cc");
	return (int)value;
}

static INLINE int bitops_fls(unsigned long value)
{
	asm volatile("bsr %1, %0" : "=r"(value) : "rm"(value) : "cc");
	return (int)value;
}

extern status_t bitmap_init(struct bitmap *b, int bits, uint8_t *data, int mmflag);
extern void bitmap_destroy(struct bitmap *b);
extern void bitmap_set(struct bitmap *b, int bit);
extern void bitmap_clear(struct bitmap *b, int bit);
extern boolean_t bitmap_test(struct bitmap *b, int bit);

#endif	/* __BITMAP_H__ */
