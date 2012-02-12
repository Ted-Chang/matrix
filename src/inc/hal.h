#ifndef __HAL_H__
#define __HAL_H__

#define MAX_GDT_DESCRIPTORS	5

/*
 * The definition of GDT entry.
 */
struct gdt {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_middle;
	uint8_t access;
	uint8_t granularity;
	uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

#define X86_MAX_INTERRUPTS	256

/*
 * The definition of IDT entry
 */
struct idt {
	uint16_t base_low;
	uint16_t sel;
	uint8_t reserved;
	uint8_t flags;
	uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed));

void outportb(uint16_t port, uint8_t value);

uint8_t inportb(uint16_t port);

uint16_t inportw(uint16_t port);

void enable_interrupt();

void disable_interrupt();

void init_gdt();

void init_idt();

/* Declaration of the interrupt service routines */
void isr0();
void isr1();
void isr2();
void isr3();
void isr4();
void isr5();
void isr6();
void isr7();
void isr8();
void isr9();
void isr10();
void isr11();
void isr12();
void isr13();
void isr14();
void isr15();
void isr16();
void isr17();
void isr18();
void isr19();
void isr20();
void isr21();
void isr22();
void isr23();
void isr24();
void isr25();
void isr26();
void isr27();
void isr28();
void isr29();
void isr30();
void isr31();

void irq0();
void irq1();
void irq2();
void irq3();
void irq4();
void irq5();
void irq6();
void irq7();
void irq8();
void irq9();
void irq10();
void irq11();
void irq12();
void irq13();
void irq14();
void irq15();

#endif	/* __HAL_H__ */
