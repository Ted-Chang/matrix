/*
 * main.c
 */

#include <types.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include "sys/time.h"
#include "matrix/matrix.h"
#include "debug.h"
#include "multiboot.h"
#include "hal/hal.h"
#include "hal/isr.h"
#include "hal/cpu.h"
#include "util.h"
#include "mm/kmem.h"
#include "mm/mmu.h"
#include "mm/malloc.h"
#include "mm/slab.h"
#include "timer.h"
#include "fs.h"
#include "initrd.h"
#include "proc/process.h"
#include "proc/sched.h"
#include "proc/thread.h"
#include "terminal.h"
#include "kd.h"
#include "keyboard.h"
#include "floppy.h"

uint32_t _initial_esp;
struct multiboot_info *_mbi;

extern void init_syscalls();

static void sys_init_thread(void *ctx);
static void dump_mbi(struct multiboot_info *mbi);

/**
 * The entry point of the matrix kernel.
 */
int kmain(u_long addr, uint32_t initial_stack)
{
	int rc = 0;

	/* Make the debugger available as soon as possible */
	init_kd();

	/* Bring up the debug terminal */
	preinit_terminal();

	/* Clear the screen */
	clear_scr();

	_mbi = (struct multiboot_info *)addr;
	ASSERT(_mbi->mods_count > 0);

	/* Dump multiboot information */
	dump_mbi(_mbi);

	_initial_esp = initial_stack;

	/* Initialize the CPU */
	preinit_cpu();
	preinit_cpu_percpu(&_boot_cpu);
	kprintf("CPU preinitialization done.\n");

	/* Enable interrupt so our timer can work */
	irq_enable();

	/* Initialize our timer */
	init_clock();
	kprintf("System PIT initialization done.\n");

	/* Initialize our memory manager */
	init_page();
	DEBUG(DL_DBG, ("Page initialization done.\n"));
	init_mmu();
	DEBUG(DL_DBG, ("MMU initialization done.\n"));
	init_kmem();
	DEBUG(DL_DBG, ("Kernel memory manager initialization done.\n"));
	init_slab();
	DEBUG(DL_DBG, ("Slab memory cache initialization done.\n"));
	init_malloc();
	DEBUG(DL_DBG, ("Kernel memory allocator initialization done.\n"));

	kprintf("Memory manager initialization done.\n");

	/* Initialize our terminal */
	init_terminal();
	kprintf("Terminal initialization done.\n");

	/* Perform more per-CPU initialization that can be done after the
	 * memory management subsystem was up
	 */
	init_cpu_percpu();
	DEBUG(DL_DBG, ("Per-CPU CPU initialization done.\n"));
	/* Properly initialize the CPU and detect other CPUs */
	init_cpu();
	DEBUG(DL_DBG, ("CPU initialization done.\n"));

	kprintf("CPU initialization done.\n");

	/* Start process sub system now */
	init_process();
	kprintf("Process initialization done.\n");

	/* Start thread sub system now */
	init_thread();
	kprintf("Thread initialization done.\n");

	/* Initialize the scheduler */
	init_sched_percpu();
	DEBUG(DL_DBG, ("Per-CPU scheduler initialization done.\n"));
	init_sched();
	DEBUG(DL_DBG, ("Scheduler initialization done.\n"));

	kprintf("Scheduler initialization done.\n");

	init_syscalls();
	kprintf("System call initialization done.\n");

	/* Create the initialization process */
	rc = thread_create("init", NULL, 0, sys_init_thread, NULL, NULL);
	ASSERT(rc == 0);

	/* Start our scheduler */
	sched_enter();

	return rc;
}

/* Load our kernel modules here */
void load_modules()
{
	init_keyboard();
	kprintf("Keyboard driver initialization done.\n");

	init_floppy();
	kprintf("Floppy driver initialization done.\n");

	vfs_type_register(&_ramfs_type);
	kprintf("Ramfs registration done.\n");
}

void sys_init_thread(void *ctx)
{
	int rc = -1;
	struct list *l;
	struct cpu *c;
	uint32_t initrd_location;
	uint32_t initrd_end;
	const char *init_argv[] = {
		"/init",
		"-d",
		NULL
	};

	/* Dump all the CPU in the system */
	LIST_FOR_EACH(l, &_running_cpus) {
		c = LIST_ENTRY(l, struct cpu, link);
		dump_cpu(c);
	}

	/* Bring up the file system manager */
	init_fs();
	kprintf("File System manager initialization done.\n");

	/* Load the modules */
	load_modules();
	if (!_root_node) {
		/* Find the location of our initial ramdisk */
		initrd_location = *((uint32_t *)_mbi->mods_addr);
		initrd_end = *(uint32_t *)(_mbi->mods_addr + 4);

		/* Initialize the initial ramdisk and set it as the root filesystem */
		_root_node = init_initrd(initrd_location);
		kprintf("Initial ramdisk mounted, location(0x%x), end(0x%x).\n",
			initrd_location, initrd_end);
	}

	/* Run init process from executable file init */
	rc = process_create(init_argv, _kernel_proc, 0, 16, NULL);
	if (rc != 0) {
		PANIC("sys_init_proc: could not start init process");
	}
}

void dump_mbi(struct multiboot_info *mbi)
{
	struct multiboot_mmap_entry *mmap;

	DEBUG(DL_DBG, ("mbi->flags: 0x%x\n", mbi->flags));
	if (FLAG_ON(mbi->flags, 0x00000001)) {
		DEBUG(DL_DBG, ("mbi->mem_low: 0x%x\n", mbi->mem_lower));
		DEBUG(DL_DBG, ("mbi->mem_upper: 0x%x\n", mbi->mem_upper));
	}
	if (FLAG_ON(mbi->flags, 0x00000002)) {
		DEBUG(DL_DBG, ("mbi->boot_dev: 0x%x\n", mbi->boot_dev));
	}
	if (FLAG_ON(mbi->flags, 0x00000004)) {
		DEBUG(DL_DBG, ("mbi->cmdline: %s\n", mbi->cmdline));
	}
	if (FLAG_ON(mbi->flags, 0x00000008)) {
		DEBUG(DL_DBG, ("mbi->mods_count: %d\n", mbi->mods_count));
		DEBUG(DL_DBG, ("mbi->mods_addr: 0x%x\n", mbi->mods_addr));
	}
	if (FLAG_ON(mbi->flags, 0x00000040)) {
		DEBUG(DL_DBG, ("mbi->mmap_length: 0x%x\n", mbi->mmap_length));
		DEBUG(DL_DBG, ("mbi->mmap_addr: 0x%x\n", mbi->mmap_addr));
	}

	for (mmap = (struct multiboot_mmap_entry *)_mbi->mmap_addr;
	     (u_long)mmap < (_mbi->mmap_addr + _mbi->mmap_length);
	     mmap = (struct multiboot_mmap_entry *)
		     ((u_long)mmap + mmap->size + sizeof(mmap->size))) {
		DEBUG(DL_DBG, ("mmap addr(0x%016lx), len(0x%016lx), type(%d)\n",
			       mmap->addr, mmap->len, mmap->type));
	}

	DEBUG(DL_DBG, ("placement address: 0x%x\n\n", *((uint32_t *)(_mbi->mods_addr + 4))));
}

