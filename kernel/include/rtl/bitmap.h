#ifndef __BITMAP_H__
#define __BITMAP_H__

/* Bitmap structure */
struct bitmap {
	u_long nr_bits;
	u_long *buf;
};

static INLINE void bitmap_init(struct bitmap *b, u_long *buf, u_long nr_bits)
{
	b->nr_bits = nr_bits;
	b->buf = buf;
}

extern void bitmap_set(struct bitmap *b, u_long bit);
extern void bitmap_clear(struct bitmap *b, u_long bit);
extern boolean_t bitmap_test(struct bitmap *b, u_long bit);
extern void bitmap_clear_all(struct bitmap *b);
extern void bitmap_set_all(struct bitmap *b);
extern void dump_bitmap(struct bitmap *b);

#endif	/* __BITMAP_H__ */
