#ifndef __ISR_H__
#define __ISR_H__

#define IRQ0	32
#define IRQ1	33
#define IRQ2	34
#define IRQ3	35
#define IRQ4	36
#define IRQ5	37
#define IRQ6	38
#define IRQ7	39
#define IRQ8	40
#define IRQ9	41
#define IRQ10	42
#define IRQ11	43
#define IRQ12	44
#define IRQ13	45
#define IRQ14	46
#define IRQ15	47

/*
 * Definitions for traps/exceptions numbers.
 */
#define X86_TRAP_DE	0	// Divide Error
#define X86_TRAP_DB	1	// Debug
#define X86_TRAP_NMI	2	// Non-maskable Interrupt
#define X86_TRAP_BP	3	// Breakpoint
#define X86_TRAP_OF	4	// Overflow
#define X86_TRAP_BR	5	// Bound Range Exceeded
#define X86_TRAP_UD	6	// Invalid Opcode
#define X86_TRAP_NM	7	// Device Not Available
#define X86_TRAP_DF	8	// Double Fault
#define X86_TRAP_OLD_MF	9	// Coprocessor Segment Overrun
#define X86_TRAP_TS	10	// Invalid TSS
#define X86_TRAP_NP	11	// Segment Not Present
#define X86_TRAP_SS	12	// Stack Segment Fault
#define X86_TRAP_GP	13	// General Protection Fault
#define X86_TRAP_PF	14	// Page Fault
#define X86_TRAP_SPURIOUS 15	// Spurious Interrupt
#define X86_TRAP_MF	16	// x87 Floating-Point Exception
#define X86_TRAP_AC	17	// Alignment Check
#define X86_TRAP_MC	18	// Machine Check
#define X86_TRAP_XF	19	// SIMD Floating-Point Exception
#define X86_TRAP_IRET	32	// IRET Exception

/*
 * Note that gs, fs segment register was reserved for other
 * purpose, so we don't put it here
 */
struct registers {
	uint32_t es;
	uint32_t ds;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t int_no;
	uint32_t err_code;
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
	uint32_t user_esp;
	uint32_t ss;
};

typedef void (*isr_t)(struct registers *);

/*
 * IRQ hook chain for precess the interrupt
 */
struct irq_desc {
	struct irq_desc *next;
	isr_t handler;
	uint8_t irq;
};

extern void init_IRQs();
extern void dump_registers(struct registers *regs);
extern void register_irq_handler(uint8_t irq, struct irq_desc *desc, isr_t handler);
extern void unregister_irq_handler(struct irq_desc *desc);

/* Declaration of the interrupt service routines */
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();
extern void isr128();	// Interrupt handler for system call

extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();
extern void irq240();	// Interrupt handler for APIC
extern void irq241();
extern void irq242();

#endif	/* __ISR_H__ */
