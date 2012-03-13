#include <types.h>
#include "isr.h"
#include "debug.h"

void divide_by_zero_fault(struct registers *regs)
{
	PANIC("Divide by zero");
}

void single_step_trap(struct registers *regs)
{
	PANIC("Single step");
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

void init_exception_handlers()
{
	/* Install the exception handlers */
	register_interrupt_handler(0, divide_by_zero_fault);
	register_interrupt_handler(1, single_step_trap);
	register_interrupt_handler(2, nmi_trap);
	register_interrupt_handler(3, breakpoint_trap);
	register_interrupt_handler(4, overflow_trap);
	register_interrupt_handler(5, bounds_check_fault);
	register_interrupt_handler(6, invalid_opcode_fault);
	register_interrupt_handler(7, no_device_fault);
	register_interrupt_handler(8, double_fault_abort);
	register_interrupt_handler(10, invalid_tss_fault);
	register_interrupt_handler(11, no_segment_fault);
	register_interrupt_handler(12, stack_fault);
	register_interrupt_handler(13, general_protection_fault);
	register_interrupt_handler(16, fpu_fault);
	register_interrupt_handler(17, alignment_check_fault);
	register_interrupt_handler(18, machine_check_abort);
	register_interrupt_handler(19, simd_fpu_fault);
}
