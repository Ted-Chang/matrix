#include <types.h>
#include <stddef.h>
#include <string.h>
#include "hal/spinlock.h"
#include "list.h"
#include "cpu.h"
#include "mutex.h"
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "mm/mm.h"
#include "mm/page.h"
#include "mm/mlayout.h"
#include "mm/kmem.h"
#include "multiboot.h"

#define INDEX_FROM_BIT(a)	(a/(8*4))
#define OFFSET_FROM_BIT(a)	(a%(8*4))


/* Placement address indicates the end of the physical memory we used */
phys_addr_t _placement_addr;

/* Total physical pages */
static page_num_t _nr_total_pages = 0;

/* Bit map for all the pages */
uint32_t *_pages;

static void set_frame(uint32_t frame_addr)
{
	uint32_t frame = frame_addr/0x1000;
	uint32_t idx = INDEX_FROM_BIT(frame);
	uint32_t off = OFFSET_FROM_BIT(frame);

	_pages[idx] |= (0x1 << off);
}

static void clear_frame(uint32_t frame_addr)
{
	uint32_t frame = frame_addr/0x1000;
	uint32_t idx = INDEX_FROM_BIT(frame);
	uint32_t off = OFFSET_FROM_BIT(frame);

	_pages[idx] &= ~(0x1 << off);
}

static uint32_t test_frame(uint32_t frame_addr)
{
	uint32_t frame = frame_addr/0x1000;
	uint32_t idx = INDEX_FROM_BIT(frame);
	uint32_t off = OFFSET_FROM_BIT(frame);

	return (_pages[idx] & (0x1 << off));
}

static uint32_t first_frame()
{
	uint32_t i, j;

	for (i = 0; i < INDEX_FROM_BIT(_nr_total_pages); i++) {
		if (_pages[i] != 0xFFFFFFFF) {
			for (j = 0; j < 32; j++) {
				uint32_t to_test = 0x1 << j;
				if (!(_pages[i] & to_test)) {
					return i*4*8+j;
				}
			}
		}
	}

	return 0;
}

void page_alloc(struct page *p, int is_kernel, int is_writable)
{
	if (p->frame != 0) {
		DEBUG(DL_DBG, ("alloc_frame: page(0x%x), frame(0x%x), kernel(%d), writable(%d)\n",
			       p, p->frame, is_kernel, is_writable));
		return;
	} else {
		/* Get the first free frame from our global frame set */
		uint32_t idx = first_frame();
		if (idx == (uint32_t)(-1)) {
			PANIC("No free frames!\n");
		}
		
		/* Mark the frame as being used */
		set_frame(idx * 0x1000);
		
		p->present = 1;
		p->rw = is_writable ? 1 : 0;
		p->user = is_kernel ? 0 : 1;
		p->frame = idx;
	}
}

void page_free(struct page *p)
{
	uint32_t frame;
	if (!(frame = p->frame)) {
		return;
	} else {
		clear_frame(frame);
		p->frame = 0x0;
	}
}

void init_page()
{
	phys_size_t mem_size;
	struct multiboot_mmap_entry *mmap;

	/* As we have only module loaded, so the end of the module is our
	 * placement address begins
	 */
	_placement_addr = *((uint32_t *)(_mbi->mods_addr + 4));

	/* Detect the amount of physical memory by parse the memory map entry */
	mem_size = 0;
	for (mmap = _mbi->mmap_addr;
	     (u_long)mmap < (_mbi->mmap_addr + _mbi->mmap_length);
	     mmap = (struct multiboot_mmap_entry *)
		     ((u_long)mmap + mmap->size + sizeof(mmap->size))) {
		mem_size += mmap->len;
	}

	DEBUG(DL_DBG, ("Detected physical memory size: %u MB.\n",
		       mem_size / (1024 * 1024)));

	_nr_total_pages = mem_size / PAGE_SIZE;

	/* Allocate the bitmap for the pages */
	_pages = kmem_alloc(_nr_total_pages/(4*8));
	memset(_pages, 0, _nr_total_pages/(4*8));
}
