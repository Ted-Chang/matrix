#include <types.h>
#include "isr.h"
#include "util.h"

void divide_by_zero_fault(struct registers regs)
{
	PANIC("Divide by zero");
	for (; ; ) ;
}

void single_step_trap(struct registers regs)
{
	PANIC("Single step");
	for (; ; ) ;
}

void nmi_trap(struct registers regs)
{
	PANIC("NMI trap");
	for (; ; ) ;
}

void breakpoint_trap(struct registers regs)
{
	PANIC("Breakpoint trap");
	for (; ; ) ;
}

void overflow_trap(struct registers regs)
{
	PANIC("Overflow trap");
	for (; ; ) ;
}

void bounds_check_fault(struct registers regs)
{
	PANIC("Bounds check fault");
	for (; ; ) ;
}

void invalid_opcode_fault(struct registers regs)
{
	PANIC("Invalid opcode fault");
	for (; ; ) ;
}

void no_device_fault(struct registers regs)
{
	PANIC("Device not found");
	for (; ; ) ;
}

void double_fault_abort(struct registers regs)
{
	PANIC("Double fault");
	for (; ; ) ;
}

void invalid_tss_fault(struct registers regs)
{
	PANIC("Invalid TSS");
	for (; ; ) ;
}

void no_segment_fault(struct registers regs)
{
	PANIC("Invalid segment");
	for (; ; ) ;
}

void stack_fault(struct registers regs)
{
	PANIC("Stack fault");
	for (; ; ) ;
}

void general_protection_fault(struct registers regs)
{
	PANIC("General protection fault");
	for (; ; ) ;
}

void fpu_fault(struct registers regs)
{
	PANIC("FPU fault");
	for (; ; ) ;
}

void alignment_check_fault(struct registers regs)
{
	PANIC("Alignment fault");
	for (; ; ) ;
}

void machine_check_abort(struct registers regs)
{
	PANIC("Machine check fault");
	for (; ; ) ;
}

void simd_fpu_fault(struct registers regs)
{
	PANIC("SIMD FPU fault");
	for (; ; ) ;
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
