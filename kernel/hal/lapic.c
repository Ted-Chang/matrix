#include <types.h>
#include <string.h>
#include "hal/core.h"
#include "hal/hal.h"
#include "debug.h"

/* Local APIC mapping. NULL if LAPIC is not present */
static volatile uint32_t *_lapic_mapping = NULL;

/* Local APIC base address */
static phys_addr_t  _lapic_base = 0;

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
}
