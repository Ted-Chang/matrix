#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "cpu.h"
#include "lapic.h"

/* Local APIC mapping. If NULL the LAPIC is not present */
static volatile uint32_t *_lapic_mapping = NULL;

/* Local APIC base address */
static uint32_t _lapic_base = 0;

extern struct cpu_features _cpu_features;


static INLINE uint32_t lapic_read(unsigned reg)
{
	return _lapic_mapping[reg];
}

static INLINE void lapic_write(unsigned reg, uint32_t value)
{
	_lapic_mapping[reg] = value;
}

static INLINE void lapic_eoi()
{
	lapic_write(LAPIC_REG_EOI, 0);
}

uint32_t lapic_id()
{
	return _lapic_mapping ? (lapic_read(LAPIC_REG_APIC_ID) >> 24) : 0;
}

/**
 * Initialize the local APIC on the current CPU.
 */
void lapic_init()
{
	uint64_t base;

	/* Don't do anything if we don't have LAPIC support */
	if (!_cpu_features.apic)
		return;

	/* Get the base address of the LAPIC mapping. If bit 11 is 0, the LAPIC
	 * is disabled.
	 */
	base = x86_read_msr(X86_MSR_APIC_BASE);
	if (!(base & (1 << 11)))
		return;
	else if (_cpu_features.x2apic && (base & (1 << 10)))
		PANIC("Cannot handle CPU in x2APIC mode");

	base &= 0xFFFFF000;
}
