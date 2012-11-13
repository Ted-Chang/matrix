#include <types.h>
#include <stddef.h>
#include "matrix/debug.h"
#include "dbgheap.h"

/* Fixed heap tag structure */
struct dbg_heap_tag {
	struct dbg_heap_tag *next;	// Next tag
	size_t data;
};
typedef struct dbg_heap_tag dbg_heap_tag_t;

static INLINE boolean_t tag_allocated(struct dbg_heap_tag *tag)
{
	return (tag->data & (1 << 0));
}

static INLINE size_t tag_size(struct dbg_heap_tag *tag)
{
	return (tag->data & ~(1 << 0));
}

void *dbg_heap_alloc(struct dbg_heap *heap, size_t size)
{
	struct dbg_heap_tag *tag, *other;
	size_t total;

	if (!size) {
		return NULL;
	}

	/* Minimum size and alignment of 8 bytes */
	total = sizeof(struct dbg_heap_tag) + ROUND_UP(size, 8);

	/* Search for a free segment */
	for (tag = heap->tags; tag; tag = tag->next) {
		if (tag_allocated(tag) || tag_size(tag) < total) {
			continue;
		}

		/* Found a suitable segment, chop it up if necessary  */
		if (tag_size(tag) > total &&
		    (tag_size(tag) - total) > (sizeof(struct dbg_heap_tag) + 8)) {
			other = (struct dbg_heap_tag *)((u_char *)tag + total);
			other->next = tag->next;
			other->data = tag_size(tag) - total;
			tag->next = other;
			tag->data = total;
		}

		/* Mark as allocated */
		tag->data |= (1 << 0);
		return (void *)((u_char *)tag + sizeof(struct dbg_heap_tag))
	}
	
	return NULL;
}

void dbg_heap_free(struct dbg_heap *heap, void *ptr)
{
	struct dbg_heap_tag *tag, *prev;

	if (!ptr) {
		return;
	}

	tag = (struct dbg_heap_tag *)((u_char *)ptr - sizeof(struct dbg_heap_tag));

	if (!tag_allocated(tag)) {
		PANIC("Freeing already free segment");
	}

	/* Mark as free */
	tag->data &= ~(1<<0);

	/* Check if there is a free tag after that we can merge with */
	if (tag->next && !tag_allocated(tag->next)) {
		tag->data += tag->next->data;
		tag->next = tag->next->next;
	}

	/* Find the previous tag and see if we can merge with it */
	if (tag != heap->tags) {
		for (prev = heap->tags; prev; prev = prev->next) {
			if (prev->next != tag) {
				continue;
			}

			if (!tag_allocated(prev)) {
				prev->data += tag->data;
				prev->next = tag->next;
			}

			return;
		}

		/* If we haven't seen a tag where next equal the tag being freed,
		 * this is an invalid free
		 */
		PANIC("Allocation does not come from heap");
	}
}

void init_dbg_heap(struct dbg_heap *heap, void *mem, size_t size)
{
	ASSERT(size >= (sizeof(struct dbg_heap_tag)) + 8);
	ASSERT((size % 2) == 0);

	/* Create an initial free segment covering the entire chunk */
	heap->tags = mem;
	heap->tags->next = NULL;
	heap->tags->data = size;
}
