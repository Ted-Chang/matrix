#include <types.h>
#include <stddef.h>
#include <string.h>
#include "rtl/rtlutil.h"
#include "hal/hal.h"
#include "hal/core.h"
#include "hal/lapic.h"
#include "mm/mmu.h"
#include "mm/phys.h"
#include "acpi.h"
#include "platform.h"

/* Whether ACPI is supported */
boolean_t _acpi_supported = FALSE;

static struct acpi_rsdp *acpi_find_rsdp(phys_addr_t start, size_t size)
{
	size_t i;
	int mflags = 0;
	struct acpi_rsdp *rsdp;

	ASSERT(!(start % 16) && !(size % 16));

	/* Map the pages to the specified address, we don't modify the content,
	 * so it's OK to share content.
	 */
	mflags = (MAP_READ_F | MAP_FIXED_F | MAP_SHARE_F);
	rsdp = phys_map(start, size, mflags);
	
	/* Search through the range on 16-byte boundaries */
	for (i = 0; i < size; i+= 16) {

		/* Check if the signature and checksum are correct */
		if (strncmp((char *)rsdp->signature, ACPI_RSDP_SIGNATURE, 8) != 0) {
			continue;
		} else {
			DEBUG(DL_INF, ("acpi: ACPI_RSDP_SIGNATURE found.\n"));
		}

		if (verify_chksum(rsdp, 20)) {
			continue;
		} else {
			DEBUG(DL_INF, ("acpi: RSDP checksum pass.\n"));
		}

		/* If the revision is 2 or higher, then check the extended
		 * field as well.
		 */
		if (rsdp->revision >= 2) {
			if (verify_chksum(rsdp, rsdp->length)) {
				DEBUG(DL_WRN,
				      ("acpi: RSDP revision(%d) checksum failed.\n",
				       rsdp->revision));
				continue;
			}
		}

		kprintf("acpi: found ACPI RSDP at 0x%llx, revision:%d\n",
			start + i, rsdp->revision);
		break;
	}

	if (i >= size) {
		phys_unmap((void *)start, size, TRUE);
		rsdp = NULL;
	}

	return rsdp;
}

void acpi_init()
{
	uint16_t *mapping;
	phys_addr_t ebda;
	struct acpi_rsdp *rsdp;

	/* Get the base address of the Extended BIOS Data Area (EBDA) */
	mapping = phys_map(0x40e, sizeof(uint16_t), 0);
	ebda = (*mapping) << 4;
	phys_unmap(mapping, sizeof(uint16_t), TRUE);
	
	kprintf("acpi: Extended BIOS Data Area at %p\n", ebda);

	/* Search for the RSDP */
	if (!(rsdp = acpi_find_rsdp(ebda, 0x400))) {
		if (!(rsdp = acpi_find_rsdp(0xE0000, 0x20000))) {
			kprintf("acpi: *** RSDP not found ***\n");
			return;
		}
	}
}

void init_platform()
{
	acpi_init();
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
