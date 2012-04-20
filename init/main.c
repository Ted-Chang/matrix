/*
 * main.c
 */

#include <types.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "multiboot.h"
#include "hal/hal.h"
#include "cpu.h"
#include "mm/kheap.h"
#include "mm/mmgr.h"
#include "mm/page.h"
#include "mm/mmu.h"
#include "mm/kmem.h"
#include "timer.h"
#include "fs.h"
#include "initrd.h"
#include "proc/task.h"
#include "exceptn.h"
#include "syscall.h"
#include "keyboard.h"
#include "floppy.h"
#include "system.h"
#include "../tests/test.h"

uint32_t _initial_esp;
struct multiboot_info *_mbi;

static void announce();
static void dump_mbi(struct multiboot_info *mbi);

/**
 * The entry point of the matrix kernel.
 */
int kmain(u_long addr, uint32_t initial_stk)
{
	int i, rc, parent_pid;
	char str[128];

	_mbi = (struct multiboot_info *)addr;

	/* Clear the screen */
	clear_scr();

	ASSERT(_mbi->mods_count > 0);

	//dump_mbi(_mbi);

	list_test();

	while (1);
	
	/* Preinitialize the CPUs in the system */
	preinit_cpu();
	preinit_per_cpu(&_boot_cpu);

	kprintf("CPU preinitialization complete!\n");

	/* Initialize kernel memory management subsystem */
	init_page();
	init_mmu();
	init_mmu_per_cpu();
	init_kmem();

	kprintf("Kernel memory management subsystem initialization complete.\n");

	/* Perform more per-CPU initialization that can be done after the
	 * memory management subsystem was up.
	 */
	//init_per_cpu();

	/* Get the system clock */
	//init_clock();

	/* Properly initialize the CPU subsystem, and detect other CPUs */
	//init_cpu();
	//init_sched();
	//init_multitask();

	/* /\* Initialize the initial ramdisk and set it as the root filesystem *\/ */
	/* root_node = init_initrd(initrd_location); */

	/* kprintf("Initial ramdisk mounted, location(0x%x), end(0x%x).\n", */
	/* 	initrd_location, initrd_end); */

	/* init_syscalls(); */

	/* kprintf("System call initialized.\n"); */

	/* init_keyboard(); */

	/* kprintf("Keyboard driver initialized.\n"); */

	/* init_floppy(); */

	/* kprintf("Floppy driver initialized.\n"); */

	/* Print the banner */
	announce();

	/* parent_pid = getpid(); */

	/* /\* Run all boot tasks *\/ */
	/* for (i = 0; i < NR_BOOT_TASKS; i++) { */
	/* 	tp = images[i]; */
	/* 	rc = fork(); */
	/* 	/\* Fork a new task and execute the specified boot task if it */
	/* 	 * was the child task */
	/* 	 *\/ */
	/* 	if (getpid() != parent_pid) { */
	/* 		tp(); */
	/* 	} */
	/* } */

	/* init_task(); */

	return rc;
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
}
