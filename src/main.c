/*
 * main.c
 */

#include "types.h"
#include "util.h"
#include "hal.h"
#include "timer.h"
#include "isr.h"
#include "kheap.h"

extern isr_t interrupt_handlers[];

int main(struct multiboot *mboot_ptr)
{
	uint32_t a, b, c;
	
	/* Install the gdt and idt */
	init_gdt();
	init_idt();
	memset(&interrupt_handlers, 0, 256*sizeof(isr_t));

	/* Clear the screen */
	clear_scr();

	/* Print the banner */
	kprintf("Welcome to Matrix!\n");

	a = kmalloc(8);

	init_paging();

	/* Test our heap management routines */
	b = kmalloc(16);
	c = kmalloc(256);

	kprintf("a:%x, b:%x, c:%x\n", a, b, c);

	kfree((void *)b);
	kfree((void *)c);

	return 0;
}
