#include <types.h>
#include <stddef.h>
#include "hal/hal.h"
#include "hal/core.h"
#include "platform.h"

void init_platform()
{
	;
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
