#include "types.h"
#include "isr.h"	// register interrupt handler
#include "util.h"	// PANIC
#include "mmgr.h"

#define INDEX_FROM_BIT(a)	(a/(8*4))
#define OFFSET_FROM_BIT(a)	(a%(8*4))

uint32_t *frames;
uint32_t nframes;

struct pd *kernel_dir = 0;
struct pd *current_dir = 0;

extern uint32_t placement_addr;

static void set_frame(uint32_t frame_addr)
{
	uint32_t frame = frame_addr/0x1000;
	uint32_t idx = INDEX_FROM_BIT(frame);
	uint32_t off = OFFSET_FROM_BIT(frame);

	frames[idx] |= (0x1 << off);
}

static void clear_frame(uint32_t frame_addr)
{
	uint32_t frame = frame_addr/0x1000;
	uint32_t idx = INDEX_FROM_BIT(frame);
	uint32_t off = OFFSET_FROM_BIT(frame);

	frames[idx] &= ~(0x1 << off);
}

static uint32_t test_frame(uint32_t frame_addr)
{
	uint32_t frame = frame_addr/0x1000;
	uint32_t idx = INDEX_FROM_BIT(frame);
	uint32_t off = OFFSET_FROM_BIT(frame);

	return (frames[idx] & (0x1 << off));
}

static uint32_t first_frame()
{
	uint32_t i, j;

	for (i = 0; i < INDEX_FROM_BIT(nframes); i++) {
		if (frames[i] != 0xFFFFFFFF) {
			for (j = 0; j < 32; j++) {
				uint32_t to_test = 0x1 << j;
				if (!(frames[i] & to_test)) {
					return i*4*8+j;
				}
			}
		}
	}

	return 0;
}

void alloc_frame(struct pte *pte, int is_kernel, int is_writable)
{
	if (pte->frame != 0)
		return;
	else {
		uint32_t idx = first_frame();
		if (idx == (uint32_t)(-1)) {
			;
		}
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
	if (!(frame = pte->frame))
		return;
	else {
		clear_frame(frame);
		pte->frame = 0x0;
	}
}

void init_paging()
{
	/*
	 * The size of physical memory. We assume it is 32 MB
	 */
	int i;
	uint32_t mem_end_page = 0x2000000;

	nframes = mem_end_page / 0x1000;
	frames = (uint32_t *)kmalloc(INDEX_FROM_BIT(nframes));
	memset(frames, 0, INDEX_FROM_BIT(nframes));

	/* Lets make a page directory */
	kernel_dir = (struct pd *)kmalloc_a(sizeof(struct pd));
	memset(kernel_dir, 0, sizeof(struct pd));
	current_dir = kernel_dir;

	i = 0;
	while (i < placement_addr) {
		/* Kernel code is readable but not writable from userspace */
		alloc_frame(get_pte(i, 1, kernel_dir), 0, 0);
		i += 0x1000;
	}

	/* Before we enable paging, we must register our page fault handler */
	register_interrupt_handler(14, page_fault);

	/* Enable paging now */
	switch_page_dir(kernel_dir);
}

void switch_page_dir(struct pd *dir)
{
	uint32_t cr0;
	current_dir = dir;
	asm volatile("mov %0, %%cr3":: "r"(&dir->physical_tables));
	asm volatile("mov %%cr0, %0": "=r"(cr0));
	cr0 |= 0x80000000;
	asm volatile("mov %0, %%cr0":: "r"(cr0));
}

struct pte *get_pte(uint32_t addr, int make, struct pd *dir)
{
	uint32_t table_idx;
	
	/* Turn on address into an index */
	addr /= 0x1000;

	/* Find the page table containing this address */
	table_idx = addr / 1024;

	if (dir->tables[table_idx])
		return &dir->tables[table_idx]->pages[addr%1024];
	else if (make) {
		uint32_t tmp;
		dir->tables[table_idx] = (struct pte *)kmalloc_ap(sizeof(struct pte), &tmp);
		memset(dir->tables[table_idx], 0, 0x1000);
		dir->physical_tables[table_idx] = tmp | 0x7;	// PRESENT, RW, US.
		return &dir->tables[table_idx]->pages[addr % 1024];
	} else
		return 0;
}

void page_fault(struct registers regs)
{
	/*
	 * A page fault has occurred. The CR2 register
	 * contains the faulting address.
	 */
	uint32_t faulting_addr;
	int present;
	int rw;
	int us;
	int reserved;
	int id;

	asm volatile("mov %%cr2, %0" : "=r"(faulting_addr));

	present = !(regs.err_code & 0x1);
	rw = regs.err_code & 0x2;
	us = regs.err_code & 0x4;
	reserved = regs.err_code & 0x8;
	id = regs.err_code & 0x10;

	/* Print an error message */
	kprintf("Page fault(%s %s %s %s) at 0x%x\n", 
		present ? "present" : "",
		rw ? "read-only" : "",
		us ? "user-mode" : "",
		reserved ? "reserved" : "",
		faulting_addr);

	PANIC("Page fault");
}
