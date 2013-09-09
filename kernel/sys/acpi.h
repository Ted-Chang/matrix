#ifndef __ACPI_H__
#define __ACPI_H__

#include <types.h>

/* Signature definitions */
#define ACPI_RSDP_SIGNATURE	"RSD PTR"	// RSDP signature
#define ACPI_MADT_SIGNATURE	"APIC"		// MADT signature
#define ACPI_DSDT_SIGNATURE	"DSDT"		// DSDT signature
#define ACPI_ECDT_SIGNATURE	"ECDT"		// ECDT signature
#define ACPI_FACP_SIGNATURE	"FACP"		// FACP signature
#define ACPI_FACS_SIGNATURE	"FACS"		// FACS signature

struct acpi_rsdp {
	uint8_t signature[8];			// Signature (ACPI_RSDP_SIGNATURE)
	uint8_t chksum;				// Checksum of first 20 bytes
	uint8_t oem_id[6];			// OEM ID string
	uint8_t revision;			// ACPI revision number
	uint32_t rsdt_addr;			// Address of RSDT
	uint32_t length;			// Length of RSDT in bytes
	uint64_t xsdt_addr;			// Address of XSDT
	uint8_t ext_chksum;			// Checksum of entire table
	uint8_t reserved[3];			// Reserved field
};

extern void acpi_init();

#endif	/* __ACPI_H__ */
