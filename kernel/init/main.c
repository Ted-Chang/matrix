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
#include "util.h"
#include "multiboot.h"
#include "hal/hal.h"
#include "hal/isr.h"
#include "hal/core.h"
#include "mm/kmem.h"
#include "mm/mmu.h"
#include "mm/malloc.h"
#include "mm/slab.h"
#include "mm/va.h"
#include "timer.h"
#include "smp.h"
#include "proc/process.h"
#include "proc/sched.h"
#include "proc/thread.h"
#include "terminal.h"
#include "kd.h"
#include "fs.h"
#include "module.h"
#include "platform.h"
#include "symbol.h"
#include "kstrdup.h"
#include "device.h"

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
	/* At least we must have the initial ramdisk loaded */
	ASSERT(_mbi->mods_count > 0);
	kprintf("mbi(0x%x) mods count(%d), mods address(0x%x).\n",
		_mbi, _mbi->mods_count, _mbi->mods_addr);

	/* Dump multiboot information */
	dump_mbi(_mbi);

	_initial_esp = initial_stack;

	/* Initialize the CPU CORE */
	preinit_core();
	kprintf("CORE preinitialization... done.\n");

	/* Enable interrupt so our timer can work */
	local_irq_enable();
	kprintf("Interrupt enabled.\n");

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

	init_va();
	kprintf("Virtual address space manager initialization... done.\n");

	/* Initialize our terminal */
	init_terminal();
	kprintf("Terminal initialization... done.\n");

	/* Properly initialize the CORE and detect other COREs */
	init_core();
	kprintf("CORE initialization... done.\n");

	/* Initialize the platform */
	init_platform();
	kprintf("Platform initialization... done.\n");

	/* Initialize Symmetric Multi Processing */
	init_smp();
	kprintf("Symmetric Multi Processing initialization... done.\n");

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
	init_sched_percore();
	kprintf("Per-CORE scheduler initialization... done.\n");
	
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

void kmain_ac(struct core *c)
{
	/* Signal that we have reached the kernel */
	_smp_boot_status = SMP_BOOT_ALIVE;

	/* Signal that we're up */
	_smp_boot_status = SMP_BOOT_BOOTED;

	/* Wait for remaining COREs to be brought up */
	while (_smp_boot_status != SMP_BOOT_COMPLETE) {
		core_spin_hint();
	}
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
	mod = kmalloc(sizeof(struct boot_module), MM_BOOT);
	LIST_INIT(&mod->link);
	mod->name = kstrdup("ramfs", 0);
	mod->handle = KMOD_RAMFS;	// TODO: use file handle in the future
	list_add_tail(&mod->link, &_boot_module_list);

	mod = kmalloc(sizeof(struct boot_module), MM_BOOT);
	LIST_INIT(&mod->link);
	mod->name = kstrdup("devfs", 0);
	mod->handle = KMOD_DEVFS;	// TODO: use file handle in the future
	list_add_tail(&mod->link, &_boot_module_list);

	mod = kmalloc(sizeof(struct boot_module), MM_BOOT);
	LIST_INIT(&mod->link);
	mod->name = kstrdup("procfs", 0);
	mod->handle = KMOD_PROCFS;	// TODO: use file handle in the future
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
	struct core *c;
	uint32_t initrd_location;
	const char *init_argv[] = {
		"/init",
		NULL
	};

	/* Dump all the CORE in the system */
	LIST_FOR_EACH(l, &_running_cores) {
		c = LIST_ENTRY(l, struct core, link);
		dump_core(c);
	}

	/* Bring up the device manager */
	init_dev();
	kprintf("Device manager initialization... done.\n");

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
		/* Find the location of our initial ramdisk */
		initrd_location = *((uint32_t *)_mbi->mods_addr);
		rc = vfs_mount(NULL, "/", "ramfs", (void *)initrd_location);
		if (rc != 0) {
			PANIC("Mount ramfs for root failed");
		}
		
		kprintf("Initial ramdisk mounted, location(0x%p).\n",
			initrd_location);

		/* Initialize the io context of kernel process */
		io_init_ctx(&_kernel_proc->ioctx, NULL);
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
		DEBUG(DL_DBG, ("mmap addr(0x%016llx), len(0x%016llx), type(%d)\n",
			       mmap->addr, mmap->len, mmap->type));
	}

	DEBUG(DL_DBG, ("placement address: 0x%x\n\n", *((uint32_t *)(_mbi->mods_addr + 4))));
}

