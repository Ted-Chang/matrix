#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "mm/fixed_heap.h"
#include "matrix/debug.h"

struct fixed_heap_tag {
	struct fixed_heap_tag *next;	// Next tag
	size_t data;			// Size and whether the tag is allocated
};

#define TAG_ALLOCATED(tag)	(tag->data & (1 << 0))

#define TAG_SIZE(tag)		(tag->data & ~(1 << 0))

void *fixed_heap_alloc(struct fixed_heap *heap, size_t size)
{
	struct fixed_heap_tag *tag, *other;
	size_t total;

	if (!size)
		return NULL;

	/* Round up the size to 8 bytes boundary */
	total = ROUND_UP(size, 8) + sizeof(struct fixed_heap_tag);

	/* Search for a free segment */
	for (tag = heap->tags; tag; tag = tag->next) {
		if (TAG_ALLOCATED(tag) || TAG_SIZE(tag) < total)
			continue;

		/* Found a suitable segment, chop it up if necessary */
		if (TAG_SIZE(tag) > total &&
		    ((TAG_SIZE(tag) - total) > (sizeof(struct fixed_heap_tag) + 8))) {
			other = (struct fixed_heap_tag *)((u_char *)tag + total);
			other->next = tag->next;
			other->data = TAG_SIZE(tag) - total;
			tag->next = other;
			tag->data = total;
		}

		/* Mark it as allocated */
		tag->data |= (1<<0);
		return (void *)((u_char *)tag + sizeof(struct fixed_heap_tag));
	}

	return NULL;
}

void fixed_heap_free(struct fixed_heap *heap, void *ptr)
{
	struct fixed_heap_tag *tag, *prev;

	if (!ptr)
		return;

	tag = (struct fixed_heap_tag *)((u_char *)ptr - sizeof(struct fixed_heap_tag));

	if (!TAG_ALLOCATED(tag))
		PANIC("Freeing already freed segment");

	/* Mark it as free */
	tag->data &= ~(1<<0);

	/* Check if there is a free tag after that we can merge with */
	if (tag->next && !TAG_ALLOCATED(tag->next)) {
		tag->data += tag->next->data;
		tag->next = tag->next->next;
	}

	/* Find the previous tag and check if we can merge with it */
	if (tag != heap->tags) {
		for (prev = heap->tags; prev; prev = prev->next) {
			if (prev->next != tag)
				continue;

			if (!TAG_ALLOCATED(prev)) {
				prev->data += tag->data;
				prev->next = tag->next;
			}

			return;
		}

		PANIC("Allocation does not come from this heap");
	}
}

void init_fixed_heap(struct fixed_heap *heap, void *mem, size_t size)
{
	ASSERT(size >= (sizeof(struct fixed_heap_tag) + 8));

	/* Create an initial free segment covering the entire chunk */
	heap->tags = mem;
	heap->tags->next = NULL;
	heap->tags->data = size;
}
