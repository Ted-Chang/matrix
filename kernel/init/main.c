/*
 * main.c
 */

#include <types.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include "sys/time.h"
#include "matrix/matrix.h"
#include "matrix/module.h"
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
#include "module.h"
#include "platform.h"
#include "symbol.h"

struct boot_module {
	struct list link;
	size_t size;
	char *name;
	int handle;		// File handle for the module data
};

/* List of modules from the loader */
static struct list _boot_module_list = {
	.prev = &_boot_module_list,
	.next = &_boot_module_list
};

uint32_t _initial_esp;
struct multiboot_info *_mbi;

extern struct vfs_mount *_root_mount;

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
	kprintf("CPU preinitialization... done.\n");

	/* Enable interrupt so our timer can work */
	irq_enable();

	/* Initialize our timer */
	init_clock();
	kprintf("System PIT initialization... done.\n");

	/* Initialize our memory manager */
	init_page();
	kprintf("Page initialization... done.\n");
	init_mmu();
	kprintf("MMU initialization... done.\n");
	init_kmem();
	kprintf("Kernel memory manager initialization... done.\n");
	init_slab();
	kprintf("Slab memory cache initialization... done.\n");
	init_malloc();
	kprintf("Kernel memory allocator initialization... done.\n");

	/* Initialize our terminal */
	init_terminal();
	kprintf("Terminal initialization... done.\n");

	/* Perform more per-CPU initialization that can be done after the
	 * memory management subsystem was up
	 */
	init_cpu_percpu();
	kprintf("Per-CPU CPU initialization... done.\n");

	/* Initialize the platform */
	init_platform();
	kprintf("Platform initialization... done.\n");

	/* Properly initialize the CPU and detect other CPUs */
	init_cpu();
	kprintf("CPU initialization... done.\n");

	/* Initialize symbol of our kernel */
	init_symbol();
	kprintf("Symbol initialization... done.\n");

	/* Start process sub system now */
	init_process();
	kprintf("Process initialization... done.\n");

	/* Start thread sub system now */
	init_thread();
	kprintf("Thread initialization... done.\n");

	/* Initialize the scheduler */
	init_sched_percpu();
	kprintf("Per-CPU scheduler initialization... done.\n");
	
	init_sched();
	kprintf("Scheduler initialization... done.\n");

	init_syscalls();
	kprintf("System call initialization... done.\n");

	/* Create the initialization process */
	rc = thread_create("init", NULL, 0, sys_init_thread, NULL, NULL);
	ASSERT(rc == 0);

	/* Start our scheduler */
	sched_enter();

	return rc;
}

static void load_boot_module(struct boot_module *mod)
{
	int rc;

	/* Try to load the module */
	rc = module_load(mod->handle);
	if (rc == 0) {
		kprintf("Load module %s... done.\n", mod->name);
	} else {
		PANIC("Load module failed");
	}
}

/* Load our kernel modules here */
void load_modules()
{
	struct list *l;
	struct boot_module *mod;

	/* Populate our module list with the module details */
	mod = kmalloc(sizeof(struct boot_module), MM_BOOT_F);
	LIST_INIT(&mod->link);
	mod->name = kmalloc(MODULE_NAME_MAX, MM_BOOT_F);
	strncpy(mod->name, "ramfs", MODULE_NAME_MAX);
	mod->handle = KMOD_RAMFS;	// TODO: use file handle in the future
	list_add_tail(&mod->link, &_boot_module_list);

	mod = kmalloc(sizeof(struct boot_module), MM_BOOT_F);
	LIST_INIT(&mod->link);
	mod->name = kmalloc(MODULE_NAME_MAX, MM_BOOT_F);
	strncpy(mod->name, "kbd", MODULE_NAME_MAX);
	mod->handle = KMOD_KBD;		// TODO: use file handle in the future
	list_add_tail(&mod->link, &_boot_module_list);

	mod = kmalloc(sizeof(struct boot_module), MM_BOOT_F);
	LIST_INIT(&mod->link);
	mod->name = kmalloc(MODULE_NAME_MAX, MM_BOOT_F);
	strncpy(mod->name, "flpy", MODULE_NAME_MAX);
	mod->handle = KMOD_FLPY;	// TODO: use file handle in the future
	list_add_tail(&mod->link, &_boot_module_list);

	mod = kmalloc(sizeof(struct boot_module), MM_BOOT_F);
	LIST_INIT(&mod->link);
	mod->name = kmalloc(MODULE_NAME_MAX, MM_BOOT_F);
	strncpy(mod->name, "devfs", MODULE_NAME_MAX);
	mod->handle = KMOD_DEVFS;	// TODO: use file handle in the future
	list_add_tail(&mod->link, &_boot_module_list);

	/* Load all kernel modules */
	LIST_FOR_EACH(l, &_boot_module_list) {
		mod = LIST_ENTRY(l, struct boot_module, link);
		load_boot_module(mod);
	}
}

/* System initialize thread */
void sys_init_thread(void *ctx)
{
	int rc = -1;
	struct list *l;
	struct cpu *c;
	uint32_t initrd_location;
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
	kprintf("File System manager initialization... done.\n");

	/* Bring up the module system manager */
	init_module();
	kprintf("Module system manager initialization... done.\n");

	/* Load the modules */
	load_modules();
	kprintf("Load boot modules... done.\n");
	
	/* Mount the ramfs at root if it was not mounted yet */
	if (!_root_mount) {
		rc = vfs_mount(NULL, "/", "ramfs", NULL);
		if (rc != 0) {
			PANIC("Mount ramfs for root failed");
		}
		
		/* Find the location of our initial ramdisk */
		initrd_location = *((uint32_t *)_mbi->mods_addr);

		/* Initialize the initial ramdisk and set it as the root filesystem */
		init_initrd(initrd_location);
		kprintf("Initial ramdisk mounted, location(0x%x).\n",
			initrd_location);
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

