#include <types.h>
#include <stddef.h>
#include "debug.h"
#include "bitmap.h"

static void dump_bitmap(struct bitmap *b)
{
	u_long i;
	boolean_t all_zeros, all_ones;
	
	kprintf("bitmap: %p (%x) %p\n", b, b->nr_bits, b->buf);

	all_zeros = FALSE;
	all_ones = FALSE;
	
	for (i = 0; i < (b->nr_bits / (sizeof(u_long) * 8)); i++) {
		if (b->buf[i] == 0) {
			if (!all_zeros) {
				kprintf("%4d: %08x\n", i, b->buf[i]);
			}

			all_zeros = TRUE;
			all_ones = FALSE;
		} else if (b->buf[i] == 0xFFFFFFFF) {
			if (!all_ones) {
				kprintf("%4d: %08x\n", i, b->buf[i]);
			}

			all_ones = TRUE;
			all_zeros = FALSE;
		} else {
			all_zeros = FALSE;
			all_ones = FALSE;

			kprintf("%4d: %08x\n", i, b->buf[i]);
		}
	}
}

void bitmap_set(struct bitmap *b, u_long bit)
{
	char *byte_addr;
	u_long shift_cnt;

	ASSERT(bit < b->nr_bits);
	byte_addr = (char *)b->buf + (bit >> 3);
	shift_cnt = bit & 0x7;
	*byte_addr |= (char)(1 << shift_cnt);
}

void bitmap_clear(struct bitmap *b, u_long bit)
{
	char *byte_addr;
	u_long shift_cnt;

	ASSERT(bit < b->nr_bits);
	byte_addr = (char *)b->buf + (bit >> 3);
	shift_cnt = bit & 0x7;
	*byte_addr &= (char)(~(1 << shift_cnt));
}

boolean_t bitmap_test(struct bitmap *b, u_long bit)
{
	char *byte_addr;
	u_long shift_cnt;

	ASSERT(bit < b->nr_bits);
	byte_addr = (char *)b->buf + (bit >> 3);
	shift_cnt = bit & 0x7;
	return ((*byte_addr & (1 << shift_cnt)) == 0) ? FALSE : TRUE;
}
