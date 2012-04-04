/*
 * mmgr.c
 */

#include <types.h>
#include "string.h"	// memset
#include "hal.h"
#include "isr.h"	// register interrupt handler
#include "mm/mmgr.h"
#include "mm/kheap.h"
#include "matrix/debug.h"

#define INDEX_FROM_BIT(a)	(a/(8*4))
#define OFFSET_FROM_BIT(a)	(a%(8*4))

/* Global memory page frame set */
uint32_t *_frames;

uint32_t _nr_frames;

struct pd *_kernel_dir = 0;
struct pd *_current_dir = 0;
static struct irq_hook _pf_hook;

extern uint32_t _placement_addr;

extern struct heap *_kheap;

extern void copy_page_physical(uint32_t dst, uint32_t src);

struct pte *get_pte(uint32_t addr, int make, struct pd *dir)
{
	uint32_t pd_idx, pt_idx;

	/* Calculate the page table index and page directory index */
	pt_idx = (addr / PAGE_SIZE) % 1024;
	pd_idx = (addr / PAGE_SIZE) / 1024;

	if (dir->tables[pd_idx]) {	// The page table already assigned
		return &dir->tables[pd_idx]->pages[pt_idx];
	} else if (make) {		// Make a new page table
		uint32_t tmp;
		
		/* Allocate a new page table */
		dir->tables[pd_idx] = (struct pt *)kmalloc_ap(sizeof(struct pt), &tmp);
		/* Clear the content of the page table */
		memset(dir->tables[pd_idx], 0, sizeof(struct pt));

		dir->physical_tables[pd_idx] = tmp | 0x7;	// PRESENT, RW, US.
		return &dir->tables[pd_idx]->pages[pt_idx];
	} else {
		DEBUG(DL_INF, ("get_pte: no pte for addr(0x%x) in dir(0x%x)\n",
			       addr, dir));
		return 0;
	}
}

/*
 * Set the bit in the bitmap to indicate the frame was used
 */
static void set_frame(uint32_t frame_addr)
{
	uint32_t frame = frame_addr/0x1000;
	uint32_t idx = INDEX_FROM_BIT(frame);
	uint32_t off = OFFSET_FROM_BIT(frame);

	_frames[idx] |= (0x1 << off);
}

/*
 * Clear the bit in the bitmap to indicate the frame was free
 */
static void clear_frame(uint32_t frame_addr)
{
	uint32_t frame = frame_addr/0x1000;
	uint32_t idx = INDEX_FROM_BIT(frame);
	uint32_t off = OFFSET_FROM_BIT(frame);

	_frames[idx] &= ~(0x1 << off);
}

/*
 * Test whether a frame is used by some modules
 */
static uint32_t test_frame(uint32_t frame_addr)
{
	uint32_t frame = frame_addr/0x1000;
	uint32_t idx = INDEX_FROM_BIT(frame);
	uint32_t off = OFFSET_FROM_BIT(frame);

	return (_frames[idx] & (0x1 << off));
}

/*
 * Find the fisrt free frame by detecting each bit in the bitmap
 */
static uint32_t first_frame()
{
	uint32_t i, j;

	for (i = 0; i < INDEX_FROM_BIT(_nr_frames); i++) {
		if (_frames[i] != 0xFFFFFFFF) {
			for (j = 0; j < 32; j++) {
				uint32_t to_test = 0x1 << j;
				if (!(_frames[i] & to_test)) {
					return i*4*8+j;
				}
			}
		}
	}

	return 0;
}

/*
 * Allocate an entry from our global memory page frame set and associated
 * a page table entry with it
 */
void alloc_frame(struct pte *pte, int is_kernel, int is_writable)
{
	if (pte->frame != 0) {
		DEBUG(DL_DBG, ("alloc_frame: pte(0x%x), frame(0x%x), kernel(%d), writable(%d)\n",
			       pte, pte->frame, is_kernel, is_writable));
		return;
	} else {
		/* Get the first free frame from our global frame set */
		uint32_t idx = first_frame();
		if (idx == (uint32_t)(-1)) {
			PANIC("No free frames!\n");
		}
		
		/* Mark the frame as being used */
		set_frame(idx * 0x1000);
		
		pte->present = 1;
		pte->rw = is_writable ? 1 : 0;
		pte->user = is_kernel ? 0 : 1;
		pte->frame = idx;
	}
}

void free_frame(struct pte *pte)
{
	uint32_t frame;
	if (!(frame = pte->frame)) {
		return;
	} else {
		clear_frame(frame);
		pte->frame = 0x0;
	}
}

void init_paging(uint64_t mem_size)
{
	/* The size of physical memory. We assume it is 32 MB */
	int i;

	/* Calculate the number of frames, 32 bit integer is enough for now. */
	_nr_frames = (uint32_t)(mem_size / PAGE_SIZE);
	_frames = (uint32_t *)kmalloc(INDEX_FROM_BIT(_nr_frames));
	memset(_frames, 0, INDEX_FROM_BIT(_nr_frames));

	/* Lets make a page directory for our kernel */
	_kernel_dir = (struct pd *)kmalloc_a(sizeof(struct pd));
	memset(_kernel_dir, 0, sizeof(struct pd));

	/* At this time paging was not enabled, so we use _kernel_dir->physical_tables
	 * as the page directory's physical address
	 */
	_kernel_dir->physical_addr = (uint32_t)_kernel_dir->physical_tables;

	DEBUG(DL_DBG, ("init_paging: kernel page directory allocated.\n"
		       "* _kernel_dir(0x%x)\n"
		       "* _kernel_dir->physical_addr(0x%x)\n",
		       _kernel_dir, _kernel_dir->physical_addr));

	/* Map some pages in the kernel heap area.
	 * Here we call get_pte but not alloc_frame. This causes page_table's
	 * to be created when necessary. We can't allocate frames yet because
	 * they need to be identity mapped first below, and yet we can't increase
	 * placement address between identity mapping and enabling heap.
	 */
	i = 0;
	for (i = KHEAP_START; i < KHEAP_START+KHEAP_INITIAL_SIZE; i+=0x1000)
		get_pte(i, TRUE, _kernel_dir);

	/* We need to identity map (physical addr == virtual addr) from
	 * 0x0 to the end of memory used by our initial ramdisk, so we
	 * can access them transparently, as if paging wasn't enabled.
	 */
	i = 0;
	while (i < (_placement_addr+0x1000)) {
		/* Kernel code is readable but not writable from user-mode */
		alloc_frame(get_pte(i, TRUE, _kernel_dir), FALSE, FALSE);
		i += 0x1000;
	}

	/* Allocate those pages we mapped earlier, our kernel heap start from
	 * address 0xC0000000 and size is 0x100000
	 */
	for (i = KHEAP_START; i < KHEAP_START+KHEAP_INITIAL_SIZE; i += 0x1000)
		alloc_frame(get_pte(i, TRUE, _kernel_dir), FALSE, FALSE);

	/* Before we enable paging, we must register our page fault handler */
	register_interrupt_handler(14, &_pf_hook, page_fault);

	/* Enable paging now */
	switch_page_dir(_kernel_dir);

	/* Initialize the kernel heap */
	_kheap = create_heap(KHEAP_START, KHEAP_START+KHEAP_INITIAL_SIZE,
			    0xCFFFF000, FALSE, FALSE);

	/* Clone the kernel page directory and switch to it, this make the kernel
	 * page directory clean.
	 */
	_current_dir = clone_pd(_kernel_dir);

	DEBUG(DL_DBG, ("init_paging: kernel page directory cloned.\n"
		       "* _current_dir(0x%x)\n"
		       "* _current_dir->physical(0x%x).\n",
		       _current_dir, _current_dir->physical_addr));

	switch_page_dir(_current_dir);
}

void switch_page_dir(struct pd *dir)
{
	uint32_t cr0;
	_current_dir = dir;

	DEBUG(DL_DBG, ("switch_page_dir: switch to page directory(0x%x)\n",
		       dir->physical_addr));
	
	asm volatile("mov %0, %%cr3":: "r"(dir->physical_addr));
	asm volatile("mov %%cr0, %0": "=r"(cr0));
	cr0 |= 0x80000000;	// Enable paging
	asm volatile("mov %0, %%cr0":: "r"(cr0));
}

/*
 * We don't call interrupt_done here, check this when we implementing
 * the paging feature of our kernel.
 */
void page_fault(struct registers *regs)
{
	uint32_t faulting_addr;
	int present;
	int rw;
	int us;
	int reserved;

	/* A page fault has occurred. The CR2 register
	 * contains the faulting address.
	 */
	asm volatile("mov %%cr2, %0" : "=r"(faulting_addr));

	present = regs->err_code & 0x1;
	rw = regs->err_code & 0x2;
	us = regs->err_code & 0x4;
	reserved = regs->err_code & 0x8;

	/* Print an error message */
	kprintf("Page fault(%s%s%s%s) at 0x%x - EIP: 0x%x\n", 
		present ? "present " : "non-present ",
		rw ? "write " : "read ",
		us ? "user-mode " : "supervisor-mode ",
		reserved ? "reserved " : "",
		faulting_addr,
		regs->eip);

	PANIC("Page fault");
}

static struct pt *clone_pt(struct pt *src, uint32_t *physical_addr)
{
	int i;
	struct pt *table;
	
	/* Make a new page table, which is page aligned */
	table = (struct pt *)kmalloc_ap(sizeof(struct pt), physical_addr);
	
	/* Clear the content of the new page table */
	memset(table, 0, sizeof(struct pt));

	DEBUG(DL_DBG, ("clone_pt: src(0x%x), table(0x%x)\n", src, table));

	for (i = 0; i < 1024; i++) {
		/* If the source entry has a frame associated with it */
		if (src->pages[i].frame) {

			/* Get a new frame */
			alloc_frame(&(table->pages[i]), FALSE, FALSE);

			/* Clone the flags from source to destination */
			if (src->pages[i].present) table->pages[i].present = 1;
			if (src->pages[i].rw) table->pages[i].rw = 1;
			if (src->pages[i].user) table->pages[i].user = 1;
			if (src->pages[i].accessed) table->pages[i].accessed = 1;
			if (src->pages[i].dirty) table->pages[i].dirty = 1;

			/* Physically copy the data accross */
			copy_page_physical(table->pages[i].frame * 0x1000, 
					   src->pages[i].frame * 0x1000);
		}
	}

	return table;
}

struct pd *clone_pd(struct pd *src)
{
	int i;
	uint32_t physical_addr;
	struct pd *dir;
	uint32_t offset;

	/* Make a new page directory and retrieve its physical address */
	dir = (struct pd *)kmalloc_ap(sizeof(struct pd), &physical_addr);
	
	/* Clear the page directory */
	memset(dir, 0, sizeof(struct pd));

	offset = (uint32_t)dir->physical_tables - (uint32_t)dir;

	/* Calculate the physical address of the page directory entry */
	dir->physical_addr = physical_addr + offset;

	DEBUG(DL_DBG, ("clone_pd: src dir(0x%x;0x%x), cloned dir(0x%x;0x%x)\n",
		       src, src->physical_addr, dir, dir->physical_addr));

	/* For each page table, if the page table is in the kernel directory,
	 * do not make a new copy.
	 */
	for (i = 0; i < 1024; i++) {
		if (!src->tables[i])
			continue;
		if (_kernel_dir->tables[i] == src->tables[i]) {
			/* It's in the kernel, so just use the same pointer */
			dir->tables[i] = src->tables[i];
			dir->physical_tables[i] = src->physical_tables[i];
		} else {
			/* Copy the page table */
			uint32_t physical;
			dir->tables[i] = clone_pt(src->tables[i], &physical);
			dir->physical_tables[i] = physical | 0x07;
		}
	}

	return dir;
}
