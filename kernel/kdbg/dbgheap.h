#ifndef __DBGHEAP_H__
#define __DBGHEAP_H__

struct dbg_heap_tag;

/* Structure containing a debug heap allocator */
struct dbg_heap {
	struct dbg_heap_tag *tags;	// List of tags
};
typedef struct dbg_heap dbg_heap_t;

extern void *dbg_heap_alloc(struct dbg_heap *heap, size_t size);
extern void dbg_heap_free(struct dbg_heap *heap, void *ptr);
extern void init_dbg_heap(struct dbg_heap *heap, void *mem, size_t size);

#endif	/* __DBGHEAP_H__ */
