/*
 * isr.c
 */

#include <types.h>
#include <stddef.h>
#include <string.h>
#include "hal/isr.h"
#include "hal/hal.h"
#include "hal/spinlock.h"
#include "hal/core.h"
#include "util.h"
#include "debug.h"

#define IRQ_COUNT	(NR_IDT_ENTRIES)

/* trap/exception handlers table */
isr_t _isr_table[NR_IDT_ENTRIES];

/* Interrupt handlers table */
isr_t _irq_table[IRQ_COUNT];

/*
 * Software interrupt handler, call the trap/exception handlers
 */
void isr_handler(struct registers regs)
{
	isr_t handler;
	uint8_t int_no;

	int_no = (uint8_t)regs.int_no;

	ASSERT(int_no < NR_IDT_ENTRIES);

	/* Call the trap/exception handler */
	handler = _isr_table[int_no];
	if (handler) {
		handler(&regs);
	} else {
		kprintf("Unknown trap/exception:%d\n", int_no);
		ASSERT(0);
	}
}

/*
 * Hardware interrupt handler, dispatch the interrupt
 */
void irq_handler(struct registers regs)
{
	isr_t handler;
	uint8_t int_no;

	/* Avoid the problem caused by the signed interrupt number if it is
	 * more than 0x80
	 */
	int_no = (uint8_t)regs.int_no;

	handler = _irq_table[int_no];
	if (handler) {
		handler(&regs);
	} else {
		kprintf("Unknown interrupt:(%d)\n", int_no);
	}

	/* Notify the PIC that we have done so we can accept >= priority
	 * IRQs now
	 */
	local_irq_done(int_no);
}

void register_IRQ(uint8_t irq, isr_t handler)
{
	if (irq >= IRQ_COUNT) {
		DEBUG(DL_ERR, ("Invalid irq number(%d)!\n", irq));
		return;
	}

	_irq_table[irq] = handler;
}

void unregister_IRQ(uint8_t irq, isr_t handler)
{
	DEBUG(DL_INF, ("unregistering irq(%d).\n"));
	_irq_table[irq] = NULL;
}

void dump_registers(struct registers *regs)
{
	kprintf("\nFaulting CORE%d:\n"
		"  eip:0x%08x esp:0x%08x ebp:0x%08x uesp:0x%08x\n"
		"  cs:0x%04x ss:0x%04x ds:0x%04x es:0x%04x\n"
		"  err_code:0x%08x int_no:0x%08x eflags:0x%08x\n\n",
		CURR_CORE->id, regs->eip, regs->esp, regs->ebp, regs->user_esp,
		regs->cs, regs->ss, regs->ds, regs->es, regs->err_code,
		regs->int_no, regs->eflags);
}

void divide_by_zero_fault(struct registers *regs)
{
	dump_registers(regs);
	PANIC("Divide by zero");
}

void single_step_fault(struct registers *regs)
{
	dump_registers(regs);
	PANIC("Single step trap");
}

void nmi_trap(struct registers *regs)
{
	dump_registers(regs);
	PANIC("NMI trap");
}

void breakpoint_trap(struct registers *regs)
{
	dump_registers(regs);
	PANIC("Breakpoint trap");
}

void overflow_trap(struct registers *regs)
{
	dump_registers(regs);
	PANIC("Overflow trap");
}

void bounds_check_fault(struct registers *regs)
{
	dump_registers(regs);
	PANIC("Bounds check fault");
}

void invalid_opcode_fault(struct registers *regs)
{
	dump_registers(regs);
	PANIC("Invalid opcode fault");
}

void no_device_fault(struct registers *regs)
{
	dump_registers(regs);
	PANIC("Device not found");
}

void double_fault_abort(struct registers *regs)
{
	dump_registers(regs);
	PANIC("Double fault");
}

void invalid_tss_fault(struct registers *regs)
{
	dump_registers(regs);
	PANIC("Invalid TSS");
}

void no_segment_fault(struct registers *regs)
{
	dump_registers(regs);
	PANIC("Invalid segment");
}

void stack_fault(struct registers *regs)
{
	dump_registers(regs);
	PANIC("Stack fault");
}

void general_protection_fault(struct registers *regs)
{
	dump_registers(regs);
	PANIC("General protection fault");
}

void fpu_fault(struct registers *regs)
{
	dump_registers(regs);
	PANIC("FPU fault");
}

void alignment_check_fault(struct registers *regs)
{
	dump_registers(regs);
	PANIC("Alignment fault");
}

void machine_check_abort(struct registers *regs)
{
	dump_registers(regs);
	PANIC("Machine check fault");
}

void simd_fpu_fault(struct registers *regs)
{
	dump_registers(regs);
	PANIC("SIMD FPU fault");
}


/*
 * Initialize the exception handlers. Exception handlers don't need to
 * call local_irq_done now as we didn't go that far.
 */
void init_IRQs()
{
	/* Initialize ISR table */
	memset(&_isr_table, 0, sizeof(_isr_table));

	/* Initialize the IRQ table */
	memset(&_irq_table, 0, sizeof(_irq_table));
	
	/* Install the exception handlers */
	_isr_table[X86_TRAP_DE] = divide_by_zero_fault;
	_isr_table[X86_TRAP_DB] = single_step_fault;
	_isr_table[X86_TRAP_NMI] = nmi_trap;
	_isr_table[X86_TRAP_BP] = breakpoint_trap;
	_isr_table[X86_TRAP_OF] = overflow_trap;
	_isr_table[X86_TRAP_BR] = bounds_check_fault;
	_isr_table[X86_TRAP_UD] = invalid_opcode_fault;
	_isr_table[X86_TRAP_NM] = no_device_fault;
	_isr_table[X86_TRAP_DF] = double_fault_abort;
	_isr_table[X86_TRAP_TS] = invalid_tss_fault;
	_isr_table[X86_TRAP_NP] = no_segment_fault;
	_isr_table[X86_TRAP_SS] = stack_fault;
	_isr_table[X86_TRAP_GP] = general_protection_fault;
	_isr_table[X86_TRAP_MF] = fpu_fault;
	_isr_table[X86_TRAP_AC] = alignment_check_fault;
	_isr_table[X86_TRAP_MC] = machine_check_abort;
	_isr_table[X86_TRAP_XF] = simd_fpu_fault;

	kprintf("IRQ dispatch table initialized.\n");
}
