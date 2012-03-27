/*
 * main.c
 */

#include <types.h>
#include <time.h>
#include "multiboot.h"
#include "string.h"
#include "util.h"
#include "hal.h"
#include "isr.h"
#include "kheap.h"
#include "mmgr.h"
#include "timer.h"
#include "fs.h"
#include "initrd.h"
#include "task.h"
#include "debug.h"
#include "exceptn.h"
#include "syscall.h"
#include "keyboard.h"
#include "floppy.h"

extern uint32_t _placement_addr;
extern struct irq_hook *_interrupt_handlers[];

uint32_t _initial_esp;

int kmain(struct multiboot *mboot_ptr, uint32_t initial_stack)
{
	int i, rc;
	uint32_t initrd_location;
	uint32_t initrd_end;
	uint64_t mem_end_page;
	struct dirent *node;
	struct tm date_time;

	/* Clear the screen */
	clear_scr();

	_initial_esp = initial_stack;

	/* Install the gdt and idt */
	init_gdt();
	init_idt();
	memset(&_interrupt_handlers[0], 0, sizeof(struct irq_hook *)*256);

	kprintf("Gdt and idt initialized.\n");

	init_exception_handlers();

	kprintf("Exception handlers installed.\n");

	/* Enable interrupt so our timer can work */
	enable_interrupt();

	/* Initialize our timer */
	init_clock();

	kprintf("Timer initialized.\n");

	ASSERT(mboot_ptr->mods_count > 0);

	/* Find the location of our initial ramdisk */
	initrd_location = *((uint32_t *)mboot_ptr->mods_addr);
	initrd_end = *(uint32_t *)(mboot_ptr->mods_addr + 4);

	/* Don't trample our module with placement address */
	_placement_addr = initrd_end;

	/* Upper memory start from 1MB and in kilo bytes */
	mem_end_page = (mboot_ptr->mem_upper + mboot_ptr->mem_lower) * 1024;
	
	/* Enable paging now */
	init_paging(mem_end_page);

	kprintf("Memory paging initialized, physical memory: %d bytes.\n",
		mem_end_page);

	/* Start multitasking now */
	init_multitask();

	kprintf("Multitask initialized.\n");

	/* Initialize the initial ramdisk and set it as the root filesystem */
	root_node = init_initrd(initrd_location);

	kprintf("Initial ramdisk initialized.\n");

	init_syscalls();

	kprintf("System call initialized.\n");

	init_keyboard();

	kprintf("Keyboard driver initialized.\n");

	init_floppy();

	kprintf("Floppy driver initialized.\n");

	/* Print the banner */
	kprintf("Welcome to Matrix!\n");

	/*Fork a new process which is a clone of this*/
	rc = fork();

	kprintf("fork returned %d\n", rc);

	kprintf("current pid: %d\n", getpid());
	
	disable_interrupt();
	
	i = 0;
	node = 0;
	
	while ((node = vfs_readdir(root_node, i)) != 0) {

		struct vfs_node *fs_node;
		
		kprintf("Found file: %s\n", node->name);

		fs_node = vfs_finddir(root_node, node->name);
		if ((fs_node->flags & 0x7) == VFS_DIRECTORY) {
			kprintf("\t(directory)\n");
		} else {
			char buf[256];
			uint32_t sz;
			int j;
			kprintf("\tcontent: ");
			sz = vfs_read(fs_node, 0, 256, (uint8_t *)buf);
			for (j = 0; j < sz; j++)
				kprintf("%c", buf[j]);
			kprintf("\n");
		}

		i++;
	}

	get_cmostime(&date_time);
	
	kprintf("year:%d, month:%d, day:%d, hour:%d, minute:%d, second:%d\n",
		date_time.tm_year, date_time.tm_mon, date_time.tm_mday,
		date_time.tm_hour, date_time.tm_min, date_time.tm_sec);

	enable_interrupt();

	//switch_to_user_mode();

	//syscall_putstr("Hello, user mode!\n");
	
	return 0;
}
