#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/matrix.h"
#include "rtl/rtlutil.h"
#include "debug.h"
#include "mm/page.h"
#include "mm/phys.h"
#include "acpi.h"

STATIC_ASSERT(sizeof(struct acpi_rsdp) == 36);
STATIC_ASSERT(sizeof(struct acpi_header) == 36);

/* Pointers to copies of ACPI tables */
static struct acpi_header **_acpi_tables = NULL;
static size_t _nr_acpi_table = 0;

static struct acpi_rsdp *acpi_find_rsdp(phys_addr_t start, size_t size)
{
	size_t i;
	struct acpi_rsdp *rsdp = NULL;

	ASSERT(!(start % 16) && !(size % 16));

	/* Search through the range on 16-byte boundaries */
	for (i = 0; i < size; i+= 16) {

		/* We have identity mapped the region before */
		rsdp = (struct acpi_rsdp *)(start + i);

		/* Check if the signature and checksum are correct */
		if (strncmp((char *)rsdp->signature, ACPI_RSDP_SIGNATURE, 8) != 0) {
			rsdp = NULL;
			continue;
		} else {
			DEBUG(DL_INF, ("acpi: ACPI_RSDP_SIGNATURE found.\n"));
		}

		if (!verify_chksum(rsdp, 20)) {
			rsdp = NULL;
			continue;
		} else {
			DEBUG(DL_INF, ("acpi: RSDP checksum pass.\n"));
		}

		/* If the revision is 2 or higher, then check the extended
		 * field as well.
		 */
		if (rsdp->revision >= 2) {
			if (!verify_chksum(rsdp, rsdp->length)) {
				DEBUG(DL_WRN,
				      ("acpi: RSDP revision(%d) checksum failed.\n",
				       rsdp->revision));
				rsdp = NULL;
				continue;
			}
		}

		kprintf("acpi: found ACPI RSDP at 0x%x, revision:%d\n",
			rsdp, rsdp->revision);
		break;
	}

	return rsdp;
}

static void acpi_copy_table(phys_addr_t addr)
{
	struct acpi_header *hdr;
	boolean_t map_success = FALSE;

	/* Try to map the address first */
	hdr = (struct acpi_header *)phys_map(addr, 2 * PAGE_SIZE, 0);
	if (hdr) {
		map_success = TRUE;
	} else {
		/* If map failed, then the page must already be mapped, so
		 * just access it directly.
		 */
		hdr = (struct acpi_header *)addr;
	}

	/* Sanity check */
	if (!verify_chksum(hdr, hdr->length)) {
		goto out;
	}

	kprintf("acpi: table %x V%d %.6s %.8s %d\n",
		addr, hdr->revision, hdr->oem_id, hdr->oem_table_id,
		hdr->oem_revision);

	//_acpi_tables[] = krealloc();
	//memcpy(_acpi_tables[], hdr, hdr->length);
	
 out:
	if (map_success) {
		phys_unmap(hdr, 2 * PAGE_SIZE, TRUE);
	}
	return;
}

static boolean_t acpi_parse_xsdt(uint32_t addr)
{
	boolean_t ret = FALSE;
	struct acpi_xsdt *xsdt;
	struct acpi_header *hdr;
	size_t i, cnt;

	/* Map 1 page table entry to the specified address */
	xsdt = (struct acpi_xsdt *)phys_map(addr, 2 * PAGE_SIZE, 0);
	ASSERT(xsdt != NULL);

	hdr = &xsdt->header;
	kprintf("acpi: XSDT %x (V%d %.6s %.8s)\n",
		xsdt, hdr->revision, hdr->oem_id, hdr->oem_table_id);

	/* Check the signature and checksum */
	if (strncmp((char *)hdr->signature, ACPI_XSDT_SIGNATURE, 4) != 0) {
		kprintf("acpi: XSDT signature not match\n");
		goto out;
	} else if (!verify_chksum(xsdt, hdr->length)) {
		kprintf("acpi: XSDT checksum incorrect\n");
		goto out;
	}

	cnt = (hdr->length - sizeof(*hdr))/sizeof(xsdt->entry[0]);
	DEBUG(DL_DBG,
	      ("acpi: XSDT %x (V%d %.6s %.8s) %d\n",
	       xsdt, hdr->revision, hdr->oem_id, hdr->oem_table_id, cnt));
	
	/* Load each table */
	for (i = 0; i < cnt; i++) {
		acpi_copy_table((phys_addr_t)xsdt->entry[i]);
	}

	ret = TRUE;

 out:
	phys_unmap(xsdt, 2 * PAGE_SIZE, TRUE);
	return ret;
}

static boolean_t acpi_parse_rsdt(uint32_t addr)
{
	boolean_t ret = FALSE;
	struct acpi_rsdt *rsdt;
	struct acpi_header *hdr;
	size_t i, cnt;

	/* Map 1 page table entry to the specified address */
	rsdt = (struct acpi_rsdt *)phys_map(addr, 2 * PAGE_SIZE, 0);
	ASSERT(rsdt != NULL);

	hdr = &rsdt->header;
	kprintf("acpi: RSDT %x (V%d %.6s %.8s)\n",
		rsdt, hdr->revision, hdr->oem_id, hdr->oem_table_id);

	/* Check the signature and checksum */
	if (strncmp((char *)hdr->signature, ACPI_RSDT_SIGNATURE, 4) != 0) {
		kprintf("acpi: RSDT signature not match\n");
		goto out;
	} else if (!verify_chksum(rsdt, hdr->length)) {
		kprintf("acpi: RSDT checksum incorrect\n");
		goto out;
	}

	cnt = (hdr->length - sizeof(*hdr))/sizeof(rsdt->entry[0]);
	DEBUG(DL_DBG,
	      ("acpi: RSDT %x (V%d %.6s %.8s) %d\n",
	       rsdt, hdr->revision, hdr->oem_id, hdr->oem_table_id, cnt));

	/* Load each table */
	for (i = 0; i < cnt; i++) {
		acpi_copy_table((phys_addr_t)rsdt->entry[i]);
	}

	ret = TRUE;

 out:
	phys_unmap(rsdt, 2 * PAGE_SIZE, TRUE);
	return ret;
}

struct acpi_header *acpi_find_table(const char *signature)
{
	size_t i;

	for (i = 0; i < _nr_acpi_table; i++) {
		if (strncmp((char *)_acpi_tables[i]->signature, signature, 4) != 0) {
			continue;
		}

		return _acpi_tables[i];
	}
	
	return NULL;
}

void init_acpi()
{
	uint16_t *mapping;
	phys_addr_t ebda;
	struct acpi_rsdp *rsdp;

	/* FixMe: As we have done identity map while initialize kernel MMU so
	 * we don't need to map it again. But the first 4K should be reserved
	 * for all address space, so we couldn't access it directly. We should
	 * do a physical map and then access it.
	 */
	
	/* Get the base address of the Extended BIOS Data Area (EBDA). */
	mapping = (uint16_t *)0x40e;
	ebda = (*mapping) << 4;
	
	kprintf("acpi: Extended BIOS Data Area at %p\n", ebda);

	/* Search for the RSDP */
	if (!(rsdp = acpi_find_rsdp(ebda, 0x400))) {
		DEBUG(DL_DBG, ("acpi: RSDP not found in EBDA\n"));
		
		/* Memory range [0xE0000, 0x100000) also included in the
		 * identity map area
		 */
		if (!(rsdp = acpi_find_rsdp(0xE0000, 0x20000))) {
			DEBUG(DL_WRN, ("acpi: *** RSDP not found ***\n"));
			kprintf("acpi: *** RSDP not found ***\n");
			return;
		} else {
			kprintf("acpi: RSDP %p (V%d %.6s)\n",
				rsdp, rsdp->revision, rsdp->oem_id);
			DEBUG(DL_DBG, ("acpi: RSDP at (%p), revision(%d), oem(%.6s)\n",
				       rsdp, rsdp->revision, rsdp->oem_id));
		}
	}

	/* Create a copy of all the tables using the XSDT where possible */
	if (rsdp->revision >= 2 && rsdp->xsdt_addr != 0) {
		if (!acpi_parse_xsdt(rsdp->xsdt_addr)) {
			if (rsdp->rsdt_addr != 0) {
				acpi_parse_rsdt(rsdp->rsdt_addr);
			}
		}
	} else if (rsdp->rsdt_addr != 0) {
		acpi_parse_rsdt(rsdp->rsdt_addr);
	} else {
		kprintf("acpi: RSDP contains neither XSDT nor RSDT address.\n");
		DEBUG(DL_WRN,
		      ("acpi: RSDP contains neither XSDT nor RSDT address\n"));
	}
}

