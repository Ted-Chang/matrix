#include <types.h>
#include <stddef.h>
#include <string.h>
#include "hal/hal.h"
#include "hal/core.h"
#include "hal/lapic.h"
#include "acpi.h"
#include "pit.h"
#include "platform.h"

/* Whether ACPI is supported */
boolean_t _acpi_supported = FALSE;

void init_platform()
{
	init_acpi();

	/* If the LAPIC is not available, we must use the PIT as the timer */
	if (!lapic_enabled()) {
		init_pit();
	}
}

void platform_reboot()
{
	uint8_t val;

	/* Try the keyboard controller */
	do {
		val = inportb(0x64);
		if (val & (1 << 0)) {
			inportb(0x60);
		}
	} while (val & (1 << 1));
	outportb(0x64, 0xfe);

	/* Fall back on a triple fault */
}

void platform_shutdown()
{
	core_halt();
}

void platform_detect_smp()
{
	struct acpi_madt_lapic *lapic;
	struct acpi_madt *madt;
	size_t len, i;
	
	/* If LAPIC is disabled, we cannot use SMP */
	if (!lapic_enabled()) {
		return;
	} else if (!_acpi_supported) {
		kprintf("platform: ACPI not supported\n");
		return;
	}

	madt = (struct acpi_madt *)acpi_find_table(ACPI_MADT_SIGNATURE);
	if (!madt) {
		kprintf("platform: Multiple ACPI Description Table not found\n");
		return;
	}

	len = madt->header.length - sizeof(struct acpi_madt);
	for (i = 0; i < len; i += lapic->length) {
		lapic = (struct acpi_madt_lapic *)(madt->apic_structures + i);
		if (lapic->type != ACPI_MADT_LAPIC) {
			continue;
		} else if (!(lapic->flags & (1<<0))) {
			/* Ignore disabled cores */
			continue;
		} else if (lapic->lapic_id == CURR_CORE->id) {
			DEBUG(DL_DBG, ("current core(%d)\n", CURR_CORE->id));
			continue;
		}

		DEBUG(DL_DBG, ("register core(%d)\n", lapic->lapic_id));

		/* Register the core */
		core_register(lapic->lapic_id, CORE_OFFLINE);
	}
}
