/*
 * main.c
 */

#include <types.h>
#include <stddef.h>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "multiboot.h"
#include "util.h"
#include "hal.h"
#include "isr.h"
#include "mm/kmem.h"
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
extern struct irq_hook *_interrupt_handlers[];

uint32_t _initial_esp;
struct multiboot_info *_mbi;

extern void init_task();

static void announce();
static void dump_mbi(struct multiboot_info *mbi);

/**
 * The entry point of the matrix kernel.
 */
int kmain(u_long addr, uint32_t initial_stack)
{
	int i, rc, parent_pid;
	uint32_t initrd_location;
	uint32_t initrd_end;
	uint64_t mem_end_page;
	task_func_t tp;

	/* Clear the screen */
	clear_scr();

	_mbi = (struct multiboot_info *)addr;
	ASSERT(_mbi->mods_count > 0);

	/* Dump multiboot information */
	dump_mbi(_mbi);

	_initial_esp = initial_stack;

	/* Install the gdt and idt */
	init_gdt();
	init_idt();
	memset(&_interrupt_handlers[0], 0, sizeof(struct irq_hook *)*256);

	kprintf("Gdt and idt installed.\n");

	init_exception_handlers();

	kprintf("Exception handlers installed.\n");

	/* Enable interrupt so our timer can work */
	enable_interrupt();

	/* Initialize our timer */
	init_clock();

	kprintf("System PIT initialized.\n");

	/* Find the location of our initial ramdisk */
	initrd_location = *((uint32_t *)_mbi->mods_addr);
	initrd_end = *(uint32_t *)(_mbi->mods_addr + 4);

	/* Initialize our memory manager */
	init_page();
	init_mmu();
	init_kmem();

	kprintf("Memory manager initialized.\n");

	/* Start multitasking now */
	init_multitask();

	kprintf("Multitask initialized.\n");

	/* Initialize the initial ramdisk and set it as the root filesystem */
	root_node = init_initrd(initrd_location);

	kprintf("Initial ramdisk mounted, location(0x%x), end(0x%x).\n",
		initrd_location, initrd_end);

	init_syscalls();

	kprintf("System call initialized.\n");

	init_keyboard();

	kprintf("Keyboard driver initialized.\n");

	init_floppy();

	kprintf("Floppy driver initialized.\n");

	/* Print the banner */
	announce();

	parent_pid = getpid();

	/* Run all boot tasks */
	for (i = 0; i < NR_BOOT_TASKS; i++) {
		tp = images[i];
		rc = fork();

		/* Fork a new task and execute the specified boot task if it
		 * was the child task
		 */
		if (getpid() != parent_pid) {
			tp(NULL);
		}
	}

	init_task(NULL);

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

	kprintf("placement address: 0x%x\n", *((uint32_t *)(_mbi->mods_addr + 4)));
}
