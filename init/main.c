/*
 * main.c
 */

#include <types.h>
#include <stddef.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "multiboot.h"
#include "hal/hal.h"
#include "hal/lirq.h"
#include "cpu.h"
#include "mm/page.h"
#include "mm/mmu.h"
#include "mm/kmem.h"
#include "mm/malloc.h"
#include "mm/vaspace.h"
#include "time.h"
#include "pit.h"
#include "clock.h"
#include "proc/process.h"
#include "proc/thread.h"
#include "proc/sched.h"
#include "fs.h"
#include "initrd.h"
#include "syscall.h"
#include "keyboard.h"
#include "floppy.h"
#include "system.h"
#include "../tests/test.h"

uint32_t _initial_esp;
struct multiboot_info *_mbi;

static void announce();
static void dump_mbi(struct multiboot_info *mbi);
static void init_thread_func(void *arg);

/**
 * The entry point of the matrix kernel.
 */
int kmain(u_long addr, uint32_t initial_stk)
{
	status_t rc;

	_mbi = (struct multiboot_info *)addr;

	/* Clear the screen */
	clear_scr();

	ASSERT(_mbi->mods_count > 0);

	//kprintf("initial_stk: 0x%x\n", initial_stk);
	//dump_mbi(_mbi);
	//unit_test();

	/* Preinitialize the CPUs in the system */
	preinit_cpu();
	preinit_per_cpu(&_boot_cpu);

	DEBUG(DL_DBG, ("CPU preinitialization complete!\n"));

	/* Initialize kernel memory management subsystem */
	init_page();
	init_mmu();
	init_mmu_per_cpu();
	init_kmem();
	init_malloc();

	DEBUG(DL_DBG, ("Memory management subsystem initialization complete!\n"));

	/* Perform more per-CPU initialization that can be done after the
	 * memory management subsystem was up.
	 */
	init_per_cpu();

	/* Get the system clock */
	init_clock();

	/* Properly initialize the CPU subsystem, and detect other CPUs */
	init_cpu();

	/* Initialize the kernel process */
	init_process();
	init_thread();
	init_sched_per_cpu();
	init_sched();

	DEBUG(DL_DBG, ("Kernel process initialization complete!\n"));

	/* Bring up the virtual memory system */
	init_va();

	DEBUG(DL_DBG, ("Virtual memory system initialization complete!\n"));

	/* Create the initialization thread */
	rc = create_thread(NULL, 0, init_thread_func, NULL, NULL);
	if (rc != STATUS_SUCCESS) {
		PANIC("Could not create initialization thread");
	}

	sched_enter();
	
	/* init_syscalls(); */

	/* kprintf("System call initialized.\n"); */

	/* init_keyboard(); */

	/* kprintf("Keyboard driver initialized.\n"); */

	/* init_floppy(); */

	/* kprintf("Floppy driver initialized.\n"); */

	/* Print the banner */
	announce();

	return rc;
}

static void init_thread_func(void *arg)
{
	while (TRUE) {
		kprintf("enter init thread.\n");
	}
}

void announce()
{
	/* Display the Matrix startup banner */
	kprintf("\nMatrix %d.%d "
		"Copyright(c) 2012, Ted Chang, Beijing, China.\n"
		"Build date and time: %s, %s\n",
		MATRIX_VERSION, MATRIX_RELEASE, __TIME__, __DATE__);
}

void dump_mbi(struct multiboot_info *mbi)
{
	struct multiboot_mmap_entry *mmap;
	
	kprintf("mbi->flags: 0x%x\n", mbi->flags);
	if (FLAG_ON(mbi->flags, 0x00000001)) {
		kprintf("mbi->mem_low: 0x%x\n", mbi->mem_lower);
		kprintf("mbi->mem_upper: 0x%x\n", mbi->mem_upper);
	}
	if (FLAG_ON(mbi->flags, 0x00000002)) {
		kprintf("mbi->boot_dev: 0x%x\n", mbi->boot_dev);
	}
	if (FLAG_ON(mbi->flags, 0x00000004)) {
		kprintf("mbi->cmdline: %s\n", mbi->cmdline);
	}
	if (FLAG_ON(mbi->flags, 0x00000008)) {
		kprintf("mbi->mods_count: %d\n", mbi->mods_count);
		kprintf("mbi->mods_addr: 0x%x\n", mbi->mods_addr);
	}
	if (FLAG_ON(mbi->flags, 0x00000040)) {
		kprintf("mbi->mmap_length: 0x%x\n", mbi->mmap_length);
		kprintf("mbi->mmap_addr: 0x%x\n", mbi->mmap_addr);
	}
	for (mmap = (struct multiboot_mmap_entry *)_mbi->mmap_addr;
	     (u_long)mmap < (_mbi->mmap_addr + _mbi->mmap_length);
	     mmap = (struct multiboot_mmap_entry *)
		     ((u_long)mmap + mmap->size + sizeof(mmap->size))) {
		kprintf("mmap addr(0x%016lx), len(0x%016lx), type(%d)\n",
			mmap->addr, mmap->len, mmap->type);
	}
	kprintf("mbi->mods_addr: 0x%x\n", *((uint32_t *)_mbi->mods_addr));
	kprintf("placement address: 0x%x\n", *((uint32_t *)(_mbi->mods_addr + 4)));
}
