/*
 * main.c
 */

#include "types.h"
#include "util.h"
#include "hal.h"
#include "timer.h"
#include "isr.h"

extern isr_t interrupt_handlers[];

int main(struct multiboot *mboot_ptr)
{
	uint32_t *ptr = (uint32_t)0xA0000000;
	uint32_t do_page_fault;
	
	/* Install the gdt and idt */
	init_gdt();
	init_idt();
	memset(&interrupt_handlers, 0, 256*sizeof(isr_t));

	/* Clear the screen */
	clear_scr();

	/* Print the banner */
	kprintf("Welcome to Matrix!\n");

	//asm volatile("int $0x3");
	//asm volatile("int $0x4");

	/* Enable the interrupt so our timer could work */
	//enable_interrupt();
	
	//init_timer(50);

	init_paging();

	kprintf("Hello, paging world!\n");

	do_page_fault = *ptr;
	
	return 0;
}
