#include "types.h"
#include "util.h"
#include "vector.h"
#include "kheap.h"
#include "mmgr.h"
#include "debug.h"

extern uint32_t end;	// end is defined in linker scripts

extern struct pd *kernel_dir;

uint32_t placement_addr = (uint32_t)&end;

struct heap *kheap = 0;

uint32_t kmalloc_int(uint32_t size, int align, uint32_t *phys)
{
	if (kheap != 0) {
		void *addr = alloc(kheap, size, (uint8_t)align);
		if (phys) {
			struct pte *page = get_pte((uint32_t)addr, 0, kernel_dir);
			*phys = page->frame * 0x1000 + ((uint32_t)addr & 0xFFF);
		}
		return (uint32_t)addr;
	} else {
		uint32_t tmp;

		/* If the address is not already page-aligned */
		if ((align == 1) && (placement_addr & 0xFFFFF000)) {
			/* Align the placement address */
			placement_addr &= 0xFFFFF000;
			placement_addr += 0x1000;
		}

		if (phys) {
			*phys = placement_addr;
		}

		tmp = placement_addr;
		placement_addr += size;
		return tmp;
	}
}

uint32_t kmalloc(uint32_t size)
{
	return kmalloc_int(size, 0, 0);
}

uint32_t kmalloc_a(uint32_t size)
{
	return kmalloc_int(size, 1, 0);
}

uint32_t kmalloc_p(uint32_t size, uint32_t *phys)
{
	return kmalloc_int(size, 0, phys);
}

uint32_t kmalloc_ap(uint32_t size, uint32_t *phys)
{
	return kmalloc_int(size, 1, phys);
}

static void expand(struct heap *heap, uint32_t new_size)
{
	uint32_t old_size, i;
	
	/* Sanity check */
	ASSERT(new_size > (heap->end_addr - heap->start_addr));

	/* Get the nearest following page boundary */
	if (new_size & 0xFFFFF000 != 0) {
		new_size &= 0xFFFFF000;
		new_size += 0x1000;
	}

	/* Make sure we're not overreaching ourselves */
	ASSERT((heap->start_addr + new_size) < heap->max_addr);

	old_size = heap->end_addr - heap->start_addr;

	i = old_size;

	while (i < new_size) {
		alloc_frame(get_pte(heap->start_addr + i, 1, kernel_dir),
			    heap->supervisor ? 1 : 0, heap->readonly ? 0 : 1);
		i += 0x1000;	/* Page size */
	}
	
	heap->end_addr = heap->start_addr + new_size;
}

static uint32_t contract(struct heap *heap, uint32_t new_size)
{
	uint32_t old_size, i;

	/* Sanity check */
	ASSERT(new_size < (heap->end_addr - heap->start_addr));

	if (new_size & 0x1000) {
		new_size &= 0x1000;
		new_size += 0x1000;
	}

	/* Don't contract too much */
	if (new_size < HEAP_MIN_SIZE)
		new_size = HEAP_MIN_SIZE;

	old_size = heap->end_addr - heap->start_addr;
	i = old_size - 0x1000;

	while (new_size < i) {
		free_frame(get_pte(heap->start_addr + i, 0, kernel_dir));
		i -= 0x1000;
	}

	heap->end_addr = heap->start_addr + new_size;
	return new_size;
}

static int8_t header_compare(void *x, void *y)
{
	struct header *a = (struct header *)x;
	struct header *b = (struct header *)y;

	if (a->size == b->size)
		return 0;
	else
		return (a->size > b->size) ? 1 : 0;
}

/*
 * create the heap
 * start - the address to place the heap index at
 */
struct heap *create_heap(uint32_t start, uint32_t end, uint32_t max,
			 uint8_t supervisor, uint8_t readonly)
{
	struct heap *heap;
	struct header *hole;

	ASSERT(start % 0x1000 == 0);
	ASSERT(end % 0x1000 == 0);

	heap = (struct heap *)kmalloc(sizeof(struct heap));

	/* Initialize the index of the heap, size of index is fixed */
	heap->index = place_vector((void *)start, HEAP_INDEX_SIZE, header_compare);

	/* Shift the start address forward to resemble where we can start putting data */
	start += sizeof(type_t) * HEAP_INDEX_SIZE;
	
	/* Make sure the start address is page-aligned */
	if (start & 0xFFFFF000 != 0) {
		start &= 0xFFFFF000;
		start += 0x1000;
	}
	
	heap->start_addr = start;
	heap->end_addr = end;
	heap->max_addr = max;
	heap->supervisor = supervisor;
	heap->readonly = readonly;

	/* Initialize header of the first hole */
	hole = (struct header *)start;
	hole->size = end - start;
	hole->magic = HEAP_MAGIC;
	hole->is_hole = 1;

	insert_vector(&heap->index, (void *)hole);

	return heap;
}

static uint32_t find_smallest_hole(struct heap *heap, uint32_t size, uint8_t page_align)
{
	uint32_t iterator;
	
	/* Find the smallest hole that will fit */
	iterator = 0;

	while (iterator < heap->index.size) {
		struct header *header = (struct header *)lookup_vector(&heap->index, iterator);
		if (page_align) {
			uint32_t location = (uint32_t)header;
			int32_t offset = 0;
			int32_t hole_size;
			
			if ((location + sizeof(struct header)) & 0xFFFFF000 != 0)
				offset = 0x1000;
			hole_size = (int32_t)header->size - offset;
			
			/* Can we fit now ? */
			if (hole_size >= (int32_t)size)
				break;
		} else if (header->size >= size) {
			break;
		}
		iterator++;
	}

	if (iterator == heap->index.size)
		return -1;	// No hole left for us
	else
		return iterator;
}

void *alloc(struct heap *heap, uint32_t size, uint8_t page_align)
{
	uint32_t new_size;
	int32_t iterator;
	struct header *orig_hole_hdr, *block_hdr;
	struct footer *block_ftr;
	uint32_t orig_hole_pos, orig_hole_size;

	new_size = sizeof(struct header) + sizeof(struct footer);
	iterator = find_smallest_hole(heap, new_size, page_align);
	if (iterator == -1) {
		uint32_t old_length = heap->end_addr - heap->start_addr;
		uint32_t old_end_addr = heap->end_addr;
		uint32_t new_length, idx, value;

		/* We need to allocate more space */
		expand(heap, old_length + new_size);
		new_length = heap->end_addr - heap->start_addr;

		/* Find the last header in location */
		iterator = 0;
		idx = -1;
		value = 0;

		while (iterator < heap->index.size) {
			uint32_t tmp = (uint32_t)lookup_vector(&heap->index, iterator);
			if (tmp > value) {
				value = tmp;
				idx = iterator;
			}
			iterator++;
		}

		if (idx == -1) {
			struct header *header = (struct header *)old_end_addr;
			struct footer *footer;
			header->magic = HEAP_MAGIC;
			header->size = new_length - old_length;
			header->is_hole = 1;
			footer = (struct footer *)(old_end_addr + header->size - sizeof(struct footer));
			footer->magic = HEAP_MAGIC;
			footer->hdr = header;
			insert_vector(&heap->index, (void *)header);
		} else {
			struct header *header = lookup_vector(&heap->index, idx);
			struct footer *footer;
			header->size += new_length - old_length;
			footer->hdr = header;
			footer->magic = HEAP_MAGIC;
		}

		/* We have enough space now. Call alloc again. */
		return alloc(heap, size, page_align);
	}

	orig_hole_hdr = (struct header *)lookup_vector(&heap->index, iterator);
	orig_hole_pos = (uint32_t)orig_hole_hdr;
	orig_hole_size = orig_hole_hdr->size;

	/*
	 * Here we work out if we should split the hole we found into parts
	 */
	if (orig_hole_size - new_size < sizeof(struct header) + sizeof(struct footer)) {
		/* Then just increase the requested size to the hole we found */
		size += orig_hole_size - new_size;
		new_size = orig_hole_size;
	}

	/*
	 * If we need to page-align the data, do it now and make a new hole
	 * in front of our block
	 */
	if (page_align && (orig_hole_pos & 0xFFFFF000)) {
		uint32_t new_location = orig_hole_pos + 0x1000 -
			(orig_hole_pos & 0xFFF) - sizeof(struct header);
		struct header *hole_header = (struct header *)orig_hole_pos;
		struct footer *hole_footer;
		hole_header->size = 0x1000;
		hole_header->magic = HEAP_MAGIC;
		hole_header->is_hole = 1;
		hole_footer = (struct footer *)(new_location - sizeof(struct footer));
		orig_hole_pos = new_location;
		orig_hole_size = orig_hole_size - hole_header->size;
	} else {
		/* We don't need this hole any more, delete it */
		remove_vector(&heap->index, iterator);
	}

	/* Overwrite the original header ... */
	block_hdr = (struct header *)orig_hole_pos;
	block_hdr->magic = HEAP_MAGIC;
	block_hdr->is_hole = 0;
	block_hdr->size = new_size;

	/* And the footer ... */
	block_ftr = (struct footer *)(orig_hole_pos + sizeof(struct header) + size);
	block_ftr->magic = HEAP_MAGIC;
	block_ftr->hdr = block_hdr;

	/*
	 * We may need to write a new hole after the allocated block.
	 */
	if ((orig_hole_size - new_size) > 0) {
		struct header *hole_hdr = (struct header *)
			(orig_hole_pos + sizeof(struct header) + size + sizeof(struct footer));
		struct footer *hole_ftr;
		hole_hdr->magic = HEAP_MAGIC;
		hole_hdr->is_hole = 1;
		hole_hdr->size = orig_hole_size - new_size;
		hole_ftr = (struct footer *)((uint32_t)hole_hdr + orig_hole_size -
					     new_size - sizeof(struct footer));
		if ((uint32_t)hole_ftr < heap->end_addr) {
			hole_ftr->magic = HEAP_MAGIC;
			hole_ftr->hdr = hole_hdr;
		}

		/* Insert the new hole into the index */
		insert_vector(&heap->index, (void *)hole_hdr);
	}

	/* Done! */
	return (void *)((uint32_t)block_hdr + sizeof(struct header));
}

void free(struct heap *heap, void *p)
{
	char do_add;
	struct header *header, *test_hdr;
	struct footer *footer, *test_ftr;
	
	if (p == 0)
		return;

	header = (struct header *)((uint32_t)p - sizeof(struct header));
	footer = (struct footer *)((uint32_t)header + header->size - sizeof(struct footer));

	/* Sanity check */
	ASSERT(header->magic == HEAP_MAGIC);
	ASSERT(header->magic == HEAP_MAGIC);

	/* Make us a hole */
	header->is_hole = 1;
	
	do_add = 1;

	test_ftr = (struct footer *)((uint32_t)header - sizeof(struct footer));
	if ((test_ftr->magic == HEAP_MAGIC) &&
	    (test_ftr->hdr->is_hole)) {
		uint32_t cache_size = header->size;	// Cache our current size
		header = test_ftr->hdr;
		footer->hdr = header;
		header->size += cache_size;
		do_add = 0;
	}

	test_hdr = (struct header *)((uint32_t)footer + sizeof(struct footer));
	if ((test_hdr->magic == HEAP_MAGIC) &&
	    (test_hdr->is_hole)) {
		uint32_t iterator;
		
		header->size += test_hdr->size;		// Increase our size
		test_ftr = (struct footer *)((uint32_t)test_hdr +
					     test_hdr->size - sizeof(struct footer));
		footer = test_ftr;
		/* Find and remove this header from the index */
		iterator = 0;
		while ((iterator < heap->index.size) &&
		       (lookup_vector(&heap->index, iterator) != (void *)test_hdr))
			iterator++;

		/* Make sure we actually find the item */
		ASSERT(iterator < heap->index.size);
		/* Remove it */
		remove_vector(&heap->index, iterator);
	}

	/* If the footer location is end address, we can contract */
	if ((uint32_t)footer + sizeof(struct footer) == heap->end_addr) {
		uint32_t old_length = heap->end_addr - heap->start_addr;
		uint32_t new_length = contract(heap, (uint32_t)header - heap->start_addr);

		if (header->size - (old_length - new_length) > 0) {
			header->size -= old_length - new_length;
			footer = (struct footer *)((uint32_t)header + header->size -
						   sizeof(struct footer));
			footer->magic = HEAP_MAGIC;
			footer->hdr = header;
		} else {

			uint32_t iterator = 0;
			while ((iterator < heap->index.size) &&
			       (lookup_vector(&heap->index, iterator) != (void *)test_hdr))
				iterator++;
			if (iterator < heap->index.size)
				remove_vector(&heap->index, iterator);
		}
	}

	if (do_add)
		insert_vector(&heap->index, (void *)header);
}


void kfree(void *p)
{
	free(kheap, p);
}
