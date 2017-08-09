#ifndef __HAL_H__
#define __HAL_H__

#include "matrix/matrix.h"
#include "hal/isr.h"
#include "hal/spinlock.h"

#define PIC1		0x20		// IO base address for master PIC
#define PIC2		0xA0		// IO base address for slave PIC
#define PIC1_CMD	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_CMD	PIC2
#define PIC2_DATA	(PIC2+1)

#define PIC_EOI		0x20		// End-Of-Interrupt command code

#define ICW1_ICW4	0x01		// ICW4 (not) needed
#define ICW1_SINGLE	0x02		// Single (cascade) mode
#define ICW1_INTERVAL4	0x04		// Call address interval 4 (8)
#define ICW1_LEVEL	0x08		// Level triggered (edge) mode
#define ICW1_INIT	0x10		// Initialization - required!

#define ICW4_8086	0x01		// 8086/88 (MCS-80/85) mode
#define ICW4_AUTO	0x02		// Auto (normal) EOI
#define ICW4_BUF_SLAVE	0x08		// Buffered mode/slave
#define ICW4_BUF_MASTER	0x0C		// Buffered mode/master
#define ICW4_SFNM	0x10		// Special fully nested (not)


#define NR_GDT_ENTRIES	6

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

#define NR_IDT_ENTRIES	256

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

/*
 * The definition of TSS entry
 */
struct tss {
	uint32_t prev_tss;	// The previous TSS
	uint32_t esp0;		// The stack pointer to load when we change to kernel
	uint32_t ss0;		// The stack segment to load when we change to kernel
	uint32_t esp1;		// Reserved...
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;		// Address of page directory for current task
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;		// The value to load into ES when we change to kernel
	uint32_t cs;		// The value to load into CS when we change to kernel
	uint32_t ss;		// The value to load into SS when we change to kernel
	uint32_t ds;		// The value to load into DS when we change to kernel
	uint32_t fs;		// The value to load into FS when we change to kernel
	uint32_t gs;		// The value to load into GS when we change to kernel
	uint32_t ldt;		// Reserved
	uint16_t trap;		// Bit 0: 0-Disabled, 1-Raise debug exception when switch
				// to task occurs
	uint16_t iomap_base;	// Offset from TSS base to I/O permissions and interrupt
				// redirection bit maps
} __attribute__((packed));

static INLINE uint8_t inportb(uint16_t port)
{
	uint8_t ret;
	asm volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

static INLINE void outportb(uint16_t port, uint8_t val)
{
	asm volatile("outb %1, %0" : : "dN" (port), "a" (val));
}

static INLINE uint16_t inportw(uint16_t port)
{
	uint16_t ret;
	asm volatile("inw %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

static INLINE void outportw(uint16_t port, uint16_t val)
{
	asm volatile("outw %1, %0" : : "dN" (port), "a" (val));
}

static INLINE uint32_t inportdw(uint16_t port)
{
	uint32_t ret;
	asm volatile("inl %1, %0" : "=a" (ret) : "dN" (port));
	return ret;
}

static INLINE void outportdw(uint16_t port, uint32_t val)
{
	asm volatile("outl %1, %0" : : "dN" (port), "a" (val));
}

extern boolean_t local_irq_enable();
extern boolean_t local_irq_disable();
extern boolean_t local_irq_state();
extern void local_irq_restore(boolean_t state);
extern void local_irq_done(uint32_t int_no);
extern void set_kernel_stack(void *stack);

#endif	/* __HAL_H__ */
