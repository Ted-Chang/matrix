#ifndef __FIXED_HEAP_H__
#define __FIXED_HEAP_H__

struct fixed_heap_tag;

struct fixed_heap {
	struct fixed_heap_tag *tags;	// List of tags
};

void *fixed_heap_alloc(struct fixed_heap *heap, size_t size);
void fixed_heap_free(struct fixed_heap *heap, void *ptr);

void init_fixed_heap(struct fixed_heap *heap, void *mem, size_t size);

#endif	/* __FIXED_HEAP_H__ */
