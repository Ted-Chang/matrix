#include <types.h>
#include <stddef.h>
#include <string.h>
#include "rtl/rtlutil.h"
#include "debug.h"
#include "acpi.h"

static struct acpi_rsdp *acpi_find_rsdp(phys_addr_t start, size_t size)
{
	size_t i;
	struct acpi_rsdp *rsdp = NULL;

	ASSERT(!(start % 16) && !(size % 16));

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

	return rsdp;
}

void acpi_init()
{
	uint16_t *mapping;
	phys_addr_t ebda;
	struct acpi_rsdp *rsdp;

	/* Get the base address of the Extended BIOS Data Area (EBDA). Note
	 * that we have done identity map while initialize kernel MMU so we
	 * don't need to map it again.
	 */
	mapping = (uint16_t *)0x40e;
	ebda = (*mapping) << 4;
	
	kprintf("acpi: Extended BIOS Data Area at %p\n", ebda);

	/* Search for the RSDP */
	if (!(rsdp = acpi_find_rsdp(ebda, 0x400))) {
		/* Memory range [0xE0000, 0x110000) also included in the
		 * identity map area
		 */
		if (!(rsdp = acpi_find_rsdp(0xE0000, 0x20000))) {
			DEBUG(DL_WRN, ("acpi: *** RSDP not found ***\n"));
			kprintf("acpi: *** RSDP not found ***\n");
			return;
		}
	}
}

