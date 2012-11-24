/*
 * isr.c
 */

#include <types.h>
#include <stddef.h>
#include "hal/isr.h"
#include "hal/hal.h"
#include "hal/spinlock.h"
#include "util.h"
#include "matrix/debug.h"

struct irq_chain {
	struct spinlock lock;
	struct irq_hook *head;
};
typedef struct irq_chain irq_chain_t;

/* Kernel Debugger hook and callback */
extern struct irq_hook _kd_hook;
extern void kd_callback(struct registers *regs);

/* Global interrupt hook chain */
static struct irq_chain _irq_chains[256];

/* Exception handler hook array */
static struct irq_hook _exceptn_hooks[17];

/*
 * Software interrupt handler, call the exception handlers
 */
void isr_handler(struct registers regs)
{
	struct irq_hook *hook;
	boolean_t processed = FALSE;
	
	/* Avoid the problem caused by the signed interrupt number if it is
	 * max than 0x80
	 */
	uint8_t int_no = regs.int_no & 0xFF;

	hook = _irq_chains[int_no].head;
	while (hook) {
		isr_t handler = hook->handler;
		if (handler) {
			handler(&regs);
		}
		hook = hook->next;
		processed = TRUE;
	}

	if (!processed) {
		kprintf("unhandled interrupt: %d\n", int_no);
		for (; ; ) ;
	}
}

/*
 * Hardware interrupt handler, dispatch the interrupt
 */
void irq_handler(struct registers regs)
{
	struct irq_hook *hook;
	boolean_t processed = FALSE;
	
	/* Notify the PIC that we have done so we can accept >= priority IRQs now */
	irq_done(regs.int_no);

	hook = _irq_chains[regs.int_no].head;
	while (hook) {
		isr_t handler = hook->handler;
		if (handler) {
			handler(&regs);
		}
		hook = hook->next;
		processed = TRUE;
	}
}

void register_irq_handler(uint8_t irq, struct irq_hook *hook, isr_t handler)
{
	struct irq_hook **line;
	
	spinlock_acquire(&_irq_chains[irq].lock);

	line = &_irq_chains[irq].head;
	while (*line) {
		/* Check if the hook has been registered already */
		if (hook == (*line)) {
			irq_enable();
			return;
		}
		line = &((*line)->next);
	}

	hook->next = NULL;
	hook->handler = handler;
	hook->irq = irq;
	*line = hook;

	spinlock_release(&_irq_chains[irq].lock);
}

void unregister_irq_handler(struct irq_hook *hook)
{
	int irq = hook->irq;
	struct irq_hook **line;

	spinlock_acquire(&_irq_chains[irq].lock);

	line = &_irq_chains[irq].head;
	while (*line) {
		if ((*line) == hook) {
			*line = (*line)->next;
			break;
		}
		
		line = &((*line)->next);
	}

	spinlock_release(&_irq_chains[irq].lock);
}

void divide_by_zero_fault(struct registers *regs)
{
	PANIC("Divide by zero");
}

void single_step_fault(struct registers *regs)
{
	PANIC("Single step trap");
}

void nmi_trap(struct registers *regs)
{
	PANIC("NMI trap");
}

void breakpoint_trap(struct registers *regs)
{
	PANIC("Breakpoint trap");
}

void overflow_trap(struct registers *regs)
{
	PANIC("Overflow trap");
}

void bounds_check_fault(struct registers *regs)
{
	PANIC("Bounds check fault");
}

void invalid_opcode_fault(struct registers *regs)
{
	kprintf("Invalid opcode CS:0x%x, EIP:0x%x\n",
		regs->cs, regs->eip);
	PANIC("Invalid opcode fault");
}

void no_device_fault(struct registers *regs)
{
	PANIC("Device not found");
}

void double_fault_abort(struct registers *regs)
{
	PANIC("Double fault");
}

void invalid_tss_fault(struct registers *regs)
{
	PANIC("Invalid TSS");
}

void no_segment_fault(struct registers *regs)
{
	PANIC("Invalid segment");
}

void stack_fault(struct registers *regs)
{
	PANIC("Stack fault");
}

void general_protection_fault(struct registers *regs)
{
	kprintf("General protection fault CS:0x%x, EIP:0x%x\n",
		regs->cs, regs->eip);
	PANIC("General protection fault");
}

void fpu_fault(struct registers *regs)
{
	PANIC("FPU fault");
}

void alignment_check_fault(struct registers *regs)
{
	PANIC("Alignment fault");
}

void machine_check_abort(struct registers *regs)
{
	PANIC("Machine check fault");
}

void simd_fpu_fault(struct registers *regs)
{
	PANIC("SIMD FPU fault");
}


/*
 * Initialize the exception handlers. Exception handlers don't need to
 * call irq_done now as we didn't go that far.
 */
void init_irqs()
{
	int i;
	
	for (i = 0; i < 256; i++) {
		spinlock_init(&_irq_chains[i].lock, "irq-lock");
		_irq_chains[i].head = NULL;
	}
	
	/* Install the exception handlers */
	register_irq_handler(0, &_exceptn_hooks[0], divide_by_zero_fault);
	register_irq_handler(1, &_exceptn_hooks[1], single_step_fault);
	register_irq_handler(2, &_exceptn_hooks[2], nmi_trap);
	register_irq_handler(3, &_exceptn_hooks[3], breakpoint_trap);
	register_irq_handler(4, &_exceptn_hooks[4], overflow_trap);
	register_irq_handler(5, &_exceptn_hooks[5], bounds_check_fault);
	register_irq_handler(6, &_exceptn_hooks[6], invalid_opcode_fault);
	register_irq_handler(7, &_exceptn_hooks[7], no_device_fault);
	register_irq_handler(8, &_exceptn_hooks[8], double_fault_abort);
	register_irq_handler(10, &_exceptn_hooks[9], invalid_tss_fault);
	register_irq_handler(11, &_exceptn_hooks[10], no_segment_fault);
	register_irq_handler(12, &_exceptn_hooks[11], stack_fault);
	register_irq_handler(13, &_exceptn_hooks[12], general_protection_fault);
	register_irq_handler(16, &_exceptn_hooks[13], fpu_fault);
	register_irq_handler(17, &_exceptn_hooks[14], alignment_check_fault);
	register_irq_handler(18, &_exceptn_hooks[15], machine_check_abort);
	register_irq_handler(19, &_exceptn_hooks[16], simd_fpu_fault);

	/* Install the serial IRQ handler */
	register_irq_handler(IRQ4, &_kd_hook, kd_callback);
}
