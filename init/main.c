/*
 * main.c
 */

#include <types.h>
#include <time.h>
#include <string.h>
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "multiboot.h"
#include "util.h"
#include "hal.h"
#include "cpu.h"
#include "mm/kheap.h"
#include "mm/mmgr.h"
#include "mm/page.h"
#include "mm/mmu.h"
#include "timer.h"
#include "fs.h"
#include "initrd.h"
#include "proc/task.h"
#include "exceptn.h"
#include "syscall.h"
#include "keyboard.h"
#include "floppy.h"
#include "system.h"

extern uint32_t _placement_addr;

uint32_t _initial_esp;

extern void init_task();

void announce();

/**
 * The entry point of the matrix kernel.
 */
int kmain(struct multiboot *mboot_ptr, uint32_t initial_stack)
{
	int i, rc, parent_pid;
	uint32_t initrd_location;
	uint32_t initrd_end;
	uint64_t mem_end_page;
	//task_func_t tp;

	ASSERT(mboot_ptr->mods_count > 0);

	_initial_esp = initial_stack;
	/* Find the location of our initial ramdisk */
	initrd_location = *((uint32_t *)mboot_ptr->mods_addr);
	initrd_end = *(uint32_t *)(mboot_ptr->mods_addr + 4);
	/* Don't trample our module with placement address */
	_placement_addr = initrd_end;
	/* Upper memory start from 1MB and in kilo bytes */
	mem_end_page = (mboot_ptr->mem_upper + mboot_ptr->mem_lower) * 1024;

	/* Clear the screen */
	clear_scr();

	/* Preinitialize the CPUs in the system */
	preinit_cpu();
	preinit_per_cpu(&_boot_cpu);

	kprintf("CPU preinitialization complete!\n");

	/* Initialize kernel memory management subsystem */
	init_page();
	init_mmu();
	init_mmu_per_cpu();
	init_kheap(mem_end_page);

	kprintf("Kernel memory management subsystem initialization complete.\n");

	/* Perform more per-CPU initialization that can be done after the
	 * memory management subsystem was up.
	 */
	init_per_cpu();

	/* Get the system clock */
	//init_clock();

	/* Properly initialize the CPU subsystem, and detect other CPUs */
	//init_cpu();
	
	/* Enable interrupt so our timer can work */
	/* enable_interrupt(); */

	/* /\* Initialize our timer *\/ */
	/* init_clock(); */

	/* kprintf("System PIT initialized.\n"); */
	
	/* /\* Start multitasking now *\/ */
	/* init_multitask(); */

	/* kprintf("Multitask initialized.\n"); */

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
