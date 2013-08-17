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
static uint32_t *_pages = NULL;
static struct spinlock _pages_lock;

static void set_frame(uint32_t frame_addr)
{
	uint32_t frame = frame_addr / 0x1000;
	uint32_t idx = INDEX_FROM_BIT(frame);
	uint32_t off = OFFSET_FROM_BIT(frame);

	spinlock_acquire(&_pages_lock);
	
	_pages[idx] |= (0x1 << off);
	
	spinlock_release(&_pages_lock);
}

static void clear_frame(uint32_t frame_addr)
{
	uint32_t frame = frame_addr / 0x1000;
	uint32_t idx = INDEX_FROM_BIT(frame);
	uint32_t off = OFFSET_FROM_BIT(frame);

	spinlock_acquire(&_pages_lock);

	_pages[idx] &= ~(0x1 << off);
	
	spinlock_release(&_pages_lock);
}

static uint32_t first_frame()
{
	uint32_t i, j, frame;

	frame = 0;

	spinlock_acquire(&_pages_lock);
	
	for (i = 0; i < INDEX_FROM_BIT(_nr_total_pages); i++) {
		if (_pages[i] != 0xFFFFFFFF) {
			for (j = 0; j < 32; j++) {
				uint32_t to_test = 0x1 << j;
				if (!(_pages[i] & to_test)) {
					frame = i * 4 * 8 + j;
					goto out;
				}
			}
		}
	}

 out:
	spinlock_release(&_pages_lock);
	
	return frame;
}

void page_alloc(struct page *p, int flags)
{
	uint32_t idx;
	
	ASSERT(p != NULL);

	if (p->frame != 0) {
		DEBUG(DL_WRN, ("page(%p), frame(%x), flags(%d)\n",
			       p, p->frame, flags));
		PANIC("alloc page in use");
	} else {
		/* Get the first free frame from our global frame set */

		idx = first_frame();
		if (idx == (uint32_t)(-1)) {
			PANIC("No free frames!\n");
		}

		/* Mark the frame as being used */
		set_frame(idx * 0x1000);

		p->present = 1;
		p->frame = idx;
	}

#ifdef _DEBUG_MM
	DEBUG(DL_DBG, ("page(%p), frame(%x).\n", p, p->frame));
#endif	/* _DEBUG_MM */
}

void page_free(struct page *p)
{
	uint32_t frame;

	ASSERT(p != NULL);

#ifdef _DEBUG_MM
	DEBUG(DL_DBG, ("page(%p), frame(%x).\n", p, p->frame));
#endif	/* _DEBUG_MM */
	
	if (!(frame = p->frame)) {
		DEBUG(DL_WRN, ("free page(%p) not allocated.\n", p));
		PANIC("free page not allocated");
	} else {
		clear_frame(frame);
		p->frame = 0;
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

	spinlock_init(&_pages_lock, "pages-lock");

	/* Calculate how many pages we have in the system */
	_nr_total_pages = mem_size / PAGE_SIZE;

	/* Allocate the bitmap for the physical pages */
	_pages = kmem_alloc(_nr_total_pages/(4*8), 0);
	ASSERT(_pages != NULL);

	/* Clear the pages bitmap */
	memset(_pages, 0, _nr_total_pages/(4*8));
}
