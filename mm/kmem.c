#include <types.h>
#include <stddef.h>
#include "status.h"
#include "list.h"
#include "matrix/debug.h"
#include "mm/mm.h"
#include "mm/mlayout.h"
#include "mm/kmem.h"
#include "mm/page.h"
#include "mm/mmu.h"
#include "vector.h"

#define HEAP_INDEX_SIZE		0x20000
#define HEAP_MAGIC		0x123890AB

phys_addr_t _pmap_addr;

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

void *kheap_alloc(struct heap *heap, size_t size, uint8_t page_align);
void *raw_alloc(size_t size, int mmflag);
void raw_free(void *addr, size_t size);

struct heap *_kheap = NULL;

void *kmem_alloc_int(size_t size, int align, uint32_t *phys)
{
	if (_kheap) {
		void *addr = kheap_alloc(_kheap, size, (uint8_t)align);
		if (phys) {
			struct page *page = mmu_get_page(&_kernel_mmu_ctx, (uint32_t)addr,
							  FALSE, 0);
			*phys = page->frame * 0x1000 + ((uint32_t)addr & 0xFFF);
		}
		return addr;
	} else {
		uint32_t tmp;

		/* If the address is not already page-aligned */
		if ((align == 1) && (_placement_addr & 0xFFFFF000)) {
			/* Align the placement address */
			_placement_addr &= 0xFFFFF000;
			_placement_addr += 0x1000;
		}

		if (phys) {
			*phys = _placement_addr;
		}

		tmp = _placement_addr;
		_placement_addr += size;
		return (void *)tmp;
	}
}

void *kmem_alloc(size_t size)
{
	return kmem_alloc_int(size, 0, 0);
}

void *kmem_alloc_a(size_t size)
{
	return kmem_alloc_int(size, 1, 0);
}

void *kmem_alloc_p(size_t size, uint32_t *phys)
{
	return kmem_alloc_int(size, 0, phys);
}

void *kmem_alloc_ap(size_t size, uint32_t *phys)
{
	return kmem_alloc_int(size, 1, phys);
}

void *kmem_map(phys_addr_t base, size_t size, int mmflag)
{
	status_t rc;
	uint32_t ret;
	size_t i;
	
	ASSERT(!(base % PAGE_SIZE) && size);

	/* Allocate virtual memory range to map the physical memory range */
	ret = (uint32_t)raw_alloc(size, 0);
	if (!ret) {
		return NULL;
	}

	mmu_lock_ctx(&_kernel_mmu_ctx);

	for (i = 0; i < size; i += PAGE_SIZE) {
		rc = mmu_map_page(&_kernel_mmu_ctx, ret + i, base + i, TRUE, TRUE, 0);
		if (rc != STATUS_SUCCESS)
			goto failed;
		DEBUG(DL_DBG, ("page mapped, virt(0x%x), base(0x%x)\n",
			       ret + i, base + i));
	}
	
	mmu_unlock_ctx(&_kernel_mmu_ctx);

	return (void *)ret;

failed:
	/* Unmap the pages we have mapped before */
	for (; i; i -= PAGE_SIZE)
		mmu_unmap_page(&_kernel_mmu_ctx, ret + (i - PAGE_SIZE), TRUE, NULL);
	
	mmu_unlock_ctx(&_kernel_mmu_ctx);
	raw_free(ret, size);
	
	return NULL;
}

static void expand(struct heap *heap, size_t new_size)
{
	uint32_t old_size, i;
	
	/* Sanity check */
	ASSERT(new_size > (heap->end_addr - heap->start_addr));

	/* Get the nearest following page boundary */
	if ((new_size & 0xFFFFF000) != 0) {
		new_size &= 0xFFFFF000;
		new_size += 0x1000;
	}

	/* Make sure we're not overreaching ourselves */
	ASSERT((heap->start_addr + new_size) < heap->max_addr);

	old_size = heap->end_addr - heap->start_addr;

	i = old_size;

	while (i < new_size) {
		page_alloc(mmu_get_page(&_kernel_mmu_ctx, heap->start_addr + i, TRUE, 0),
			   heap->supervisor ? 1 : 0, heap->readonly ? 0 : 1);
		i += 0x1000;	/* Page size */
	}
	
	heap->end_addr = heap->start_addr + new_size;
}

static uint32_t contract(struct heap *heap, size_t new_size)
{
	uint32_t old_size, i;

	/* Sanity check */
	ASSERT(new_size < (heap->end_addr - heap->start_addr));

	if (new_size & 0x1000) {
		new_size &= 0x1000;
		new_size += 0x1000;
	}

	/* Don't contract too much */
	if (new_size < KERNEL_KMEM_SIZE)
		new_size = KERNEL_KMEM_SIZE;

	old_size = heap->end_addr - heap->start_addr;
	i = old_size - 0x1000;

	while (new_size < i) {
		page_free(mmu_get_page(&_kernel_mmu_ctx, heap->start_addr + i, FALSE, 0));
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

struct heap *create_kheap(uint32_t start, uint32_t end, uint32_t max,
			  uint8_t supervisor, uint8_t readonly)
{
	struct heap *heap;
	struct header *hole;

	ASSERT(start % 0x1000 == 0);
	ASSERT(end % 0x1000 == 0);

	heap = (struct heap *)kmem_alloc(sizeof(struct heap));

	/* Initialize the index of the heap, size of index is fixed */
	heap->index = place_vector((void *)start, HEAP_INDEX_SIZE, header_compare);

	/* Shift the start address forward to resemble where we can start putting data */
	start += sizeof(type_t) * HEAP_INDEX_SIZE;
	
	/* Make sure the start address is page-aligned */
	if ((start & 0xFFFFF000) != 0) {
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

static uint32_t find_smallest_hole(struct heap *heap, size_t size, uint8_t page_align)
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
			
			if ((((location + sizeof(struct header))) & 0xFFFFF000) != 0)
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

	if (iterator == heap->index.size) {
		return -1;	// No hole left for us
	} else {
		return iterator;
	}
}

void *kheap_alloc(struct heap *heap, size_t size, uint8_t page_align)
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
			/* The last header need adjusting */
			struct header *header;
			/* Rewrite the footer */
			struct footer *footer;
			header = lookup_vector(&heap->index, idx);
			header->size += (new_length - old_length);
			footer = (struct footer *)((uint32_t)header + header->size - sizeof(struct footer));
			footer->hdr = header;
			footer->magic = HEAP_MAGIC;
		}

		/* We have enough space now. Call alloc again. */
		return kheap_alloc(heap, size, page_align);
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
		hole_footer->magic = HEAP_MAGIC;
		hole_footer->hdr = hole_header;
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

void kheap_free(struct heap *heap, void *p)
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

void *raw_alloc(size_t size, int mmflag)
{
	void *addr;
	
	ASSERT(size && (size % PAGE_SIZE == 0));

	addr = (void *)_pmap_addr;
	_pmap_addr += size;

	return addr;
}

void raw_free(void *addr, size_t size)
{
	;
}

void kmem_free(void *p)
{
	kheap_free(_kheap, p);
}

void init_kmem()
{
	_pmap_addr = KERNEL_PMAP_BASE;
	
	/* Create kernel heap at KERNEL_KMEM_BASE */
	_kheap = create_kheap(KERNEL_KMEM_BASE, KERNEL_KMEM_BASE+KERNEL_KMEM_SIZE,
			      0xCFFFF000, FALSE, FALSE);
}
