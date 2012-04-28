#include <types.h>
#include "hal/hal.h"
#include "hal/isr.h"
#include "matrix/debug.h"

static struct irq_hook _exceptn_hooks[18];

void divide_by_zero_fault(struct intr_frame *frame)
{
	PANIC("Divide by zero");
}

void single_step_trap(struct intr_frame *frame)
{
	PANIC("Single step");
}

void nmi_trap(struct intr_frame *frame)
{
	PANIC("NMI trap");
}

void breakpoint_trap(struct intr_frame *frame)
{
	PANIC("Breakpoint trap");
}

void overflow_trap(struct intr_frame *frame)
{
	PANIC("Overflow trap");
}

void bounds_check_fault(struct intr_frame *frame)
{
	PANIC("Bounds check fault");
}

void invalid_opcode_fault(struct intr_frame *frame)
{
	kprintf("Invalid opcode CS:0x%x, EIP:0x%x\n",
		frame->cs, frame->eip);
	PANIC("Invalid opcode fault");
}

void no_device_fault(struct intr_frame *frame)
{
	PANIC("Device not found");
}

void double_fault_abort(struct intr_frame *frame)
{
	PANIC("Double fault");
}

void invalid_tss_fault(struct intr_frame *frame)
{
	PANIC("Invalid TSS");
}

void no_segment_fault(struct intr_frame *frame)
{
	PANIC("Invalid segment");
}

void stack_fault(struct intr_frame *frame)
{
	PANIC("Stack fault");
}

void general_protection_fault(struct intr_frame *frame)
{
	kprintf("General protection fault CS:0x%x, EIP:0x%x\n",
		frame->cs, frame->eip);
	PANIC("General protection fault");
}

void page_fault(struct intr_frame *frame)
{
	uint32_t faulting_addr;
	int present;
	int rw;
	int us;
	int reserved;

	/* A page fault has occurred. The CR2 register
	 * contains the faulting address.
	 */
	asm volatile("mov %%cr2, %0" : "=r"(faulting_addr));

	present = frame->err_code & 0x1;
	rw = frame->err_code & 0x2;
	us = frame->err_code & 0x4;
	reserved = frame->err_code & 0x8;

	/* Print an error message */
	kprintf("Page fault(%s%s%s%s) at 0x%x - EIP: 0x%x\n", 
		present ? "present " : "non-present ",
		rw ? "write " : "read ",
		us ? "user-mode " : "supervisor-mode ",
		reserved ? "reserved " : "",
		faulting_addr,
		frame->eip);

	PANIC("Page fault");
}

void fpu_fault(struct intr_frame *frame)
{
	PANIC("FPU fault");
}

void alignment_check_fault(struct intr_frame *frame)
{
	PANIC("Alignment fault");
}

void machine_check_abort(struct intr_frame *frame)
{
	PANIC("Machine check fault");
}

void simd_fpu_fault(struct intr_frame *frame)
{
	PANIC("SIMD FPU fault");
}


/*
 * Initialize the exception handlers. Exception handlers don't need to
 * call interrupt_done now as we didn't go that far.
 */
void init_exceptn_handlers()
{
	/* Install the exception handlers */
	register_irq_handler(0, &_exceptn_hooks[0], divide_by_zero_fault);
	register_irq_handler(1, &_exceptn_hooks[1], single_step_trap);
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
	register_irq_handler(14, &_exceptn_hooks[13], page_fault);
	register_irq_handler(16, &_exceptn_hooks[14], fpu_fault);
	register_irq_handler(17, &_exceptn_hooks[15], alignment_check_fault);
	register_irq_handler(18, &_exceptn_hooks[16], machine_check_abort);
	register_irq_handler(19, &_exceptn_hooks[17], simd_fpu_fault);
}
