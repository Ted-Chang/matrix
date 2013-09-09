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
	acpi_init();

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
	/* If LAPIC is disabled, we cannot use SMP */
	if (!lapic_enabled()) {
		return;
	} else if (!_acpi_supported) {
		return;
	}
}
