/*
 * main.c
 */

#include "types.h"
#include "util.h"
#include "hal.h"
#include "timer.h"

int main(struct multiboot *mboot_ptr)
{
	/* Install the gdt and idt */
	init_gdt();
	init_idt();

	/* Clear the screen */
	clear_scr();

	/* Print the banner */
	kprintf("Welcome to Matrix!\n");

	asm volatile("int $0x3");
	asm volatile("int $0x4");

	/* Enable the interrupt so our timer could work */
	enable_interrupt();
	
	init_timer(50);

	return 0;
}
