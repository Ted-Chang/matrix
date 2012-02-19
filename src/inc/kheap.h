#ifndef __KHEAP_H__
#define __KHEAP_H__

#define KHEAP_START		0xC0000000
#define KHEAP_INITIAL_SIZE	0x100000

#define HEAP_INDEX_SIZE		0x20000
#define HEAP_MAGIC		0x123890AB
#define HEAP_MIN_SIZE		0x70000

/*
 * Size information for a block
 */
struct header {
	uint32_t magic;		// magic number, used for sanity check
	uint32_t size;
	uint8_t is_hole;
};

struct footer {
	uint32_t magic;		// magic number, same as in header
	struct header *hdr;
};

struct heap {
	struct vector index;
	uint32_t start_addr;	// start of our allocated space
	uint32_t end_addr;	// end of our allocated space
	uint32_t max_addr;	// maximum address the heap can expand to
	uint8_t supervisor; 
	uint8_t readonly;
};

struct heap *create_heap(uint32_t start, uint32_t end, uint32_t max,
			 uint8_t supervisor, uint8_t readonly);

void *alloc(struct heap *heap, uint32_t size, uint8_t page_align);

void free(struct heap *heap, void *p);

/*
 * Routines for allocate a chunk of memory
 */
uint32_t kmalloc(uint32_t size);

uint32_t kmalloc_a(uint32_t size);

uint32_t kmalloc_p(uint32_t size, uint32_t *phys);

uint32_t kmalloc_ap(uint32_t size, uint32_t *phys);

uint32_t kmalloc_int(uint32_t size, int align, uint32_t *phys);

void kfree(void *p);

#endif	/* __KHEAP_H__ */
