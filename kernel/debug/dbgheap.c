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

void *dbg_heap_alloc(struct dbg_heap *heap, size_t size)
{
	return NULL;
}

void dbg_heap_free(struct dbg_heap *heap, void *ptr)
{
	;
}

void init_dbg_heap(struct dbg_heap *heap, void *mem, size_t size)
{
	ASSERT(size >= (sizeof(struct dbg_heap_tag)) + 8);

	/* Create an initial free segment covering the entire chunk */
	heap->tags = mem;
	heap->tags->next = NULL;
	heap->tags->data = size;
}
