#include <types.h>
#include <stddef.h>
#include <string.h>
#include "mm/page.h"
#include "mm/kmem.h"
#include "multiboot.h"
#include "debug.h"

#define INDEX_FROM_BIT(a)	((a)/(8*4))
#define OFFSET_FROM_BIT(a)	((a)%(8*4))

/* Placement address indicates the end of the physical memory */
phys_addr_t _placement_addr = 0;

/* Total physical pages */
static page_num_t _nr_total_pages = 0;

/* Bitmap for all pages */
uint32_t *_pages = NULL;

static void set_frame(uint32_t frame_addr)
{
	uint32_t frame = frame_addr / 0x1000;
	uint32_t idx = INDEX_FROM_BIT(frame);
	uint32_t off = OFFSET_FROM_BIT(frame);

	_pages[idx] |= (0x1 << off);
}

static void clear_frame(uint32_t frame_addr)
{
	uint32_t frame = frame_addr / 0x1000;
	uint32_t idx = INDEX_FROM_BIT(frame);
	uint32_t off = OFFSET_FROM_BIT(frame);

	_pages[idx] &= ~(0x1 << off);
}

static uint32_t first_frame()
{
	uint32_t i, j;

	for (i = 0; i < INDEX_FROM_BIT(_nr_total_pages); i++) {
		if (_pages[i] != 0xFFFFFFFF) {
			for (j = 0; j < 32; j++) {
				uint32_t to_test = 0x1 << j;
				if (!(_pages[i] & to_test)) {
					return i * 4 * 8 + j;
				}
			}
		}
	}

	return 0;
}

void page_alloc(struct page *p, boolean_t kernel, boolean_t write)
{
	ASSERT(p != NULL);
	
	if (p->frame != 0) {
		//DEBUG(DL_DBG, ("page(0x%x), frame(0x%x), kernel(%d), write(%d)\n",
		//	       p, p->frame, kernel, write));
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
		p->rw = write ? 1 : 0;
		p->user = kernel ? 0 : 1;
		p->frame = idx;
	}
}

void page_free(struct page *p)
{
	uint32_t frame;

	ASSERT(p != NULL);
	
	if (!(frame = p->frame)) {
		DEBUG(DL_WRN, ("free page(%p) not allocated.\n", p));
		return;
	} else {
		clear_frame(frame);
		p->frame = 0x0;
		p->present = 0;
	}
}

void init_page()
{
	phys_size_t mem_size = 0;
	struct multiboot_mmap_entry *mmap;
	
	/* As we have only one module loaded, so the end of the module is our
	 * where our placement address begins
	 */
	_placement_addr = *((uint32_t *)(_mbi->mods_addr + 4));
	
	DEBUG(DL_DBG, ("Placement address: 0x%x\n", _placement_addr));

	/* Detect the amount of physical memory by parse the memory map entry */
	for (mmap = (struct multiboot_mmap_entry *)_mbi->mmap_addr;
	     (u_long)mmap < (_mbi->mmap_addr + _mbi->mmap_length);
	     mmap = (struct multiboot_mmap_entry *)
		     ((u_long)mmap + mmap->size + sizeof(mmap->size))) {
		mem_size += mmap->len;
	}

	DEBUG(DL_DBG, ("Detected physical memory size: %uMB.\n", mem_size/(1024*1024)));

	_nr_total_pages = mem_size / PAGE_SIZE;

	/* Allocate the bitmap for the physical pages */
	_pages = kmem_alloc(_nr_total_pages/(4*8), 0);
	memset(_pages, 0, _nr_total_pages/(4*8));
}
