#include <types.h>
#include <string.h>
#include "hal/core.h"
#include "hal/hal.h"
#include "hal/lapic.h"
#include "debug.h"
#include "mm/page.h"
#include "mm/phys.h"

/* Local APIC mapping. NULL if LAPIC is not present */
static volatile uint32_t *_lapic_mapping = NULL;

/* Local APIC base address */
static phys_addr_t _lapic_base = 0;

static struct irq_hook _lapic_hook[3];

static INLINE uint32_t lapic_read(uint32_t reg)
{
	return _lapic_mapping[reg];
}

static INLINE void lapic_write(uint32_t reg, uint32_t val)
{
	_lapic_mapping[reg] = val;
}

static INLINE void lapic_eoi()
{
	lapic_write(LAPIC_REG_EOI, 0);
}

void lapic_spurious_handler(struct registers *regs)
{
	kprintf("lapic: received spurious interrupt!\n");
}

void lapic_timer_handler(struct registers * regs)
{
	DEBUG(DL_INF, ("lapic: received timer interrupt\n"));
	lapic_eoi();
}

void lapic_ipi_handler(struct registers *regs)
{
	DEBUG(DL_INF, ("lapic: received ipi interrupt\n"));
	lapic_eoi();
}

boolean_t lapic_enabled()
{
	return _lapic_mapping != NULL;
}

uint32_t lapic_id()
{
	return (_lapic_mapping) ? (lapic_read(LAPIC_REG_APIC_ID) >> 24) : 0;
}

void init_lapic()
{
	uint64_t base;

	/* Don't do anything if we don't support LAPIC */
	if (!_core_features.apic) {
		return;
	}

	/* Get the base address of the LAPIC mapping. If bit 11 is 0, the
	 * LAPIC is disabled.
	 */
	base = x86_read_msr(X86_MSR_APIC_BASE);
	if (!(base & (1 << 11))) {
		return;
	} else if (_core_features.x2apic && (base & (1 << 11))) {
		PANIC("Cannot handle core in x2APIC mode");
	}

	base &= 0xFFFFF000;
	if (_lapic_mapping) {
		/* This is a secondary core. Ensure that the base address
		 * is not different from the boot core's.
		 */
		if (base != _lapic_base) {
			PANIC("Core has different LAPIC address from boot core");
		}
	} else {
		/* This is the boot core. Map the LAPIC into virtual memory and
		 * register interrupt handlers.
		 */
		_lapic_base = base;
		_lapic_mapping = phys_map(base, PAGE_SIZE, 0);
		kprintf("lapic: physical location 0x%x mapped to %p\n",
			base, _lapic_mapping);

		/* Register interrupt handlers */
		register_irq_handler(LAPIC_VECT_SPURIOUS, &_lapic_hook[0],
				     lapic_spurious_handler);
		register_irq_handler(LAPIC_VECT_TIMER, &_lapic_hook[1],
				     lapic_timer_handler);
		register_irq_handler(LAPIC_VECT_IPI, &_lapic_hook[2],
				     lapic_ipi_handler);
	}

	/* Enable the local APIC (bit 8) and set spurious interrupt handler
	 * in the Spurious Interrupt Vector Register.
	 */
	//lapic_write(LAPIC_REG_SPURIOUS, LAPIC_VECT_SPURIOUS | (1<<8));
	//lapic_write(LAPIC_REG_TIMER_DIVIDER, LAPIC_TIMER_DIV8);

	/* Sanity check */
	if (CURR_CORE != &_boot_core) {
		if (CURR_CORE->id != lapic_id()) {
			PANIC("Core ID mismatch");
		}
	}

	/* Accept all interrupts */
	//lapic_write(LAPIC_REG_TPR, lapic_read(LAPIC_REG_TPR) & 0xFFFFFF00);

	/* Enable the timer: interrupt vector, no extra bits = Unmasked/One-shot  */
	//lapic_write(LAPIC_REG_TIMER_INITIAL, 0);
	//lapic_write(LAPIC_REG_LVT_TIMER, LAPIC_VECT_TIMER);
}
