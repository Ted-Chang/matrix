/*
 * main.c
 */

#include "types.h"
#include "util.h"
#include "hal.h"
#include "timer.h"
#include "isr.h"
#include "kheap.h"
#include "multiboot.h"
#include "fs.h"
#include "initrd.h"
#include "task.h"

extern isr_t interrupt_handlers[];

extern uint32_t placement_addr;

uint32_t initial_esp;

int main(struct multiboot *mboot_ptr, uint32_t initial_stack)
{
	int i, rc;
	uint32_t initrd_location;
	uint32_t initrd_end;
	struct dirent *node;

	initial_esp = initial_stack;
	
	/* Install the gdt and idt */
	init_gdt();
	init_idt();
	memset(&interrupt_handlers, 0, 256*sizeof(isr_t));

	/* Clear the screen */
	clear_scr();

	enable_interrupt();
	
	init_timer(50);

	ASSERT(mboot_ptr->mods_count > 0);
	/* Find the location of our initial ramdisk */
	initrd_location = *((uint32_t *)mboot_ptr->mods_addr);
	initrd_end = *(uint32_t *)(mboot_ptr->mods_addr + 4);

	/* Don't trample our module with placement address */
	placement_addr = initrd_end;
	
	/* Enable paging now */
	init_paging();

	/* Start multitasking now */
	init_multitask();

	/* Initialize the initial ramdisk and set it as the filesystem root */
	root_node = init_initrd(initrd_location);

	/* Print the banner */
	kprintf("Welcome to Matrix!\n");

	/* Fork a new process which is a clone of this */
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
			kprintf("\tcontent: \"");
			sz = vfs_read(fs_node, 0, 256, buf);
			for (j = 0; j < sz; j++)
				kprintf("%c", buf[j]);
			kprintf("\"\n");
		}

		i++;
	}

	enable_interrupt();
	
	return 0;
}
