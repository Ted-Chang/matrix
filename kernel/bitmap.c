#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/debug.h"
#include "status.h"
#include "mm/kmem.h"
#include "bitmap.h"

status_t bitmap_init(struct bitmap *b, int bits, uint8_t *data, int mmflag)
{
	ASSERT(bits > 0);

	if (data) {
		b->allocated = FALSE;
		b->data = data;
	} else {
		b->data = kmem_alloc(BITMAP_BYTES(bits));
		if (!b->data)
			return STATUS_NO_MEMORY;
		b->allocated = TRUE;
		memset(b->data, 0, BITMAP_BYTES(bits));
	}
	
	b->nr_bits = bits;
	return STATUS_SUCCESS;
}

void bitmap_destroy(struct bitmap *b)
{
	if (b->allocated)
		kmem_free(b->data);
}

void bitmap_set(struct bitmap *b, int bit)
{
	ASSERT(bit < b->nr_bits);

	b->data[bit / 8] |= (1 << (bit % 8));
}

void bitmap_clear(struct bitmap *b, int bit)
{
	ASSERT(bit < b->nr_bits);

	b->data[bit / 8] &= ~(1 << (bit % 8));
}

boolean_t bitmap_test(struct bitmap *b, int bit)
{
	ASSERT(bit < b->nr_bits);

	return b->data[bit / 8] & (1 << (bit % 8));
}

int bitmap_ffs(struct bitmap *b)
{
	size_t total = b->nr_bits;
	unsigned long value;
	int result = 0;

	/* Note that we use unsigned long here because its length is platform
	 * dependent. In this way, it will be faster than use a fixed length type
	 */
	while (total >= BITS_PER_LONG) {
		value = ((unsigned long *)b->data)[result / BITS_PER_LONG];
		if (value != ~((unsigned long)0))
			return result + bitops_ffs(value);
		
		total -= BITS_PER_LONG;
		result += BITS_PER_LONG;
	}

	return -1;
}

int bitmap_ffz(struct bitmap *b)
{
	size_t total = b->nr_bits;
	unsigned long value;
	int result = 0;

	while (total >= BITS_PER_LONG) {
		value = ((unsigned long *)b->data)[result / BITS_PER_LONG];
		if (value != ~((unsigned long)0))
			return result + bitops_ffz(value);

		total -= BITS_PER_LONG;
		result += BITS_PER_LONG;
	}

	return -1;
}
