/*
 * hal.c
 */

#include <types.h>
#include "hal.h"
#include "string.h"	// memset

struct idt _idt_entries[NR_IDT_ENTRIES];
struct idt_ptr _idt_ptr;
struct tss _tss_entry;
struct gdt _gdt_entries[NR_GDT_ENTRIES];
struct gdt_ptr _gdt_ptr;

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
static void write_tss(int32_t num, uint16_t ss0, uint32_t esp0);
static void gdt_set_gate(uint32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t granu);

void idt_flush(uint32_t);
void tss_flush();
void gdt_flush(uint32_t);

void outportb(uint16_t port, uint8_t value)
{
	asm volatile("outb %1, %0" : : "dN" (port), "a" (value));
}

uint8_t inportb(uint16_t port)
{
	uint8_t ret;
	asm volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

uint16_t inportw(uint16_t port)
{
	uint16_t ret;
	asm volatile("inw %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

/*
 * Enable the hardware interrupts
 */
void enable_interrupt()
{
	asm volatile("sti");
}

/*
 * Disable the hardware interrupts
 */
void disable_interrupt()
{
	asm volatile("cli");
}

/*
 * Notify the HAL interrupt handler was done
 */
void interrupt_done(uint32_t int_no)
{
	if (int_no >= 40)
		outportb(PIC2_CMD, PIC_EOI);
	
	outportb(PIC1_CMD, PIC_EOI);
}

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
	if (num >= NR_IDT_ENTRIES)
		return;
	if (!base)
		return;

	_idt_entries[num].base_low = base & 0xFFFF;
	_idt_entries[num].base_high = (base >> 16) & 0xFFFF;

	_idt_entries[num].sel = sel;
	_idt_entries[num].reserved = 0;
	
	/* We have to uncomment the OR below when we get to using user-mode.
	 * It sets the interrupt gate's privilege level to 3.
	 */
	_idt_entries[num].flags = flags | 0x60;
}

static void pic_remap(int offset1, int offset2)
{
	/* Starts the initialization sequence (in cascade mode) */
	outportb(PIC1_CMD, ICW1_INIT|ICW1_ICW4);
	outportb(PIC2_CMD, ICW1_INIT|ICW1_ICW4);
	
	outportb(PIC1_DATA, offset1);	// ICW2: Master PIC vector offset
	outportb(PIC2_DATA, offset2);	// ICW2: Slave PIC vector offset

	/* ICW3: tell master PIC that there is a slave PIC at IRQ2 (0000 0100) */
	outportb(PIC1_DATA, 0x04);
	/* ICW3: tell slave PIC its cascade identity (0000 0010) */
	outportb(PIC2_DATA, 0x02);
	
	outportb(PIC1_DATA, ICW4_8086);
	outportb(PIC2_DATA, ICW4_8086);
	
	outportb(0x21, 0x0);
	outportb(0xA1, 0x0);
}

void init_idt()
{
	_idt_ptr.limit = sizeof(struct idt) * 256 - 1;
	_idt_ptr.base = (uint32_t)&_idt_entries;

	memset(&_idt_entries, 0, sizeof(struct idt) * 256);

	/* Remap the irq table so we will not conflict with the PIC */
	pic_remap(0x20, 0x28);
	
	/* Set the default interrupt service routine */
	idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);
	idt_set_gate(1, (uint32_t)isr1, 0x08, 0x8E);
	idt_set_gate(2, (uint32_t)isr2, 0x08, 0x8E);
	idt_set_gate(3, (uint32_t)isr3, 0x08, 0x8E);
	idt_set_gate(4, (uint32_t)isr4, 0x08, 0x8E);
	idt_set_gate(5, (uint32_t)isr5, 0x08, 0x8E);
	idt_set_gate(6, (uint32_t)isr6, 0x08, 0x8E);
	idt_set_gate(7, (uint32_t)isr7, 0x08, 0x8E);
	idt_set_gate(8, (uint32_t)isr8, 0x08, 0x8E);
	idt_set_gate(9, (uint32_t)isr9, 0x08, 0x8E);
	idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
	idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
	idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
	idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
	idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
	idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
	idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
	idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
	idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
	idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
	idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
	idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
	idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
	idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
	idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
	idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
	idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
	idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
	idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
	idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
	idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
	idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);
	
	idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);
	idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);
	idt_set_gate(34, (uint32_t)irq2, 0x08, 0x8E);
	idt_set_gate(35, (uint32_t)irq3, 0x08, 0x8E);
	idt_set_gate(36, (uint32_t)irq4, 0x08, 0x8E);
	idt_set_gate(37, (uint32_t)irq5, 0x08, 0x8E);
	idt_set_gate(38, (uint32_t)irq6, 0x08, 0x8E);
	idt_set_gate(39, (uint32_t)irq7, 0x08, 0x8E);
	idt_set_gate(40, (uint32_t)irq8, 0x08, 0x8E);
	idt_set_gate(41, (uint32_t)irq9, 0x08, 0x8E);
	idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
	idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
	idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
	idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
	idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
	idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);
	
	/* The following interrupt number is for system call */
	idt_set_gate(128, (uint32_t)isr128, 0x08, 0x8E);

	idt_flush((uint32_t)&_idt_ptr);
}

static void write_tss(int32_t num, uint16_t ss0, uint32_t esp0)
{
	uint32_t base, limit;

	/* Firstly, let's compute the base and limit of our entry into the GDT */
	base = (uint32_t)&_tss_entry;
	limit = base + sizeof(struct tss);

	/* Now, add our TSS descriptor's address to the GDT */
	gdt_set_gate(num, base, limit, 0xE9, 0x00);

	/* Ensure the descriptor is initially zero */
	memset(&_tss_entry, 0, sizeof(struct tss));

	_tss_entry.ss0 = ss0;	// Set the kernel stack segment
	_tss_entry.esp0 = esp0;	// Set the kernel stack pointer

	/* Here we set the cs, ss, ds, es, fs and gs entries in the TSS. These specify
	 * what segments should be laoded when the processor switches to kernel mode.
	 * Therefore they are just our normal kernel code/data segments - 0x08 and 0x10
	 * respectively, but with the last two bits set, making 0x0b and 0x13. The setting
	 * of these bits sets the RPL (requested privilege level) to 3, meaning that TSS
	 * can be used to switch to kernel mode from ring 3.
	 */
	_tss_entry.cs = 0x0b;
	_tss_entry.ss = _tss_entry.ds =
		_tss_entry.es =
		_tss_entry.fs =
		_tss_entry.gs =
		0x13;
}

static void gdt_set_gate(uint32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t granu)
{
	_gdt_entries[num].base_low = (base & 0xFFFF);
	_gdt_entries[num].base_middle = (base >> 16) & 0xFF;
	_gdt_entries[num].base_high = (base >> 24) & 0xFF;

	_gdt_entries[num].limit_low = (limit & 0xFFFF);
	_gdt_entries[num].granularity = (limit >> 16) & 0x0F;

	_gdt_entries[num].granularity |= granu & 0xF0;
	_gdt_entries[num].access = access;
}

static void __init_gdt()
{
	/* 5 GDT entry and a TSS entry */
	_gdt_ptr.limit = (sizeof(struct gdt) * 6) - 1;
	_gdt_ptr.base = (uint32_t)&_gdt_entries;

	/* The NULL segment */
	gdt_set_gate(0, 0, 0, 0, 0);
	
	gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);	// Kernel code segment
	gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);	// Kernel data segment
	gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);	// Usermode code segment
	gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);	// Usermode data segment

	write_tss(5, 0x10, 0);

	gdt_flush((uint32_t)&_gdt_ptr);

	/* flush the TSS */
	tss_flush();
}

void init_gdt()
{
	__init_gdt();
}

void set_kernel_stack(uint32_t stack)
{
	_tss_entry.esp0 = stack;
}
