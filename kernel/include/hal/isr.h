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
struct irq_hook {
	struct irq_hook *next;
	isr_t handler;
	uint8_t irq;
};

extern void init_irqs();
extern void dump_registers(struct registers *regs);
extern void register_irq_handler(uint8_t irq, struct irq_hook *hook, isr_t handler);
extern void unregister_irq_handler(struct irq_hook *hook);

#endif	/* __ISR_H__ */
