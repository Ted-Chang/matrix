#ifndef __ACPI_H__
#define __ACPI_H__

#include <types.h>

/* Signature definitions */
#define ACPI_RSDP_SIGNATURE	"RSD PTR "	// RSDP signature
#define ACPI_MADT_SIGNATURE	"APIC"		// MADT signature
#define ACPI_DSDT_SIGNATURE	"DSDT"		// DSDT signature
#define ACPI_ECDT_SIGNATURE	"ECDT"		// ECDT signature
#define ACPI_FACP_SIGNATURE	"FACP"		// FACP signature
#define ACPI_FACS_SIGNATURE	"FACS"		// FACS signature
#define ACPI_PSDT_SIGNATURE	"PSDT"		// PSDT signature
#define ACPI_RSDT_SIGNATURE	"RSDT"		// RSDT signature
#define ACPI_SBST_SIGNATURE	"SBST"		// SBST signature
#define ACPI_SLIT_SIGNATURE	"SLIT"		// SLIT signature
#define ACPI_SRAT_SIGNATURE	"SRAT"		// SRAT signature
#define ACPI_SSDT_SIGNATURE	"SSDT"		// SSDT signature
#define ACPI_XSDT_SIGNATURE	"XSDT"		// XSDT signature

/* MADT APIC types */
#define ACPI_MADT_LAPIC		0		// Core Local APIC
#define ACPI_MADT_IOAPIC	1		// I/O APIC

/* Root System Description Pointer (RSDP) */
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

struct acpi_header {
	uint8_t signature[4];			// Signature
	uint32_t length;			// Length of header
	uint8_t revision;			// ACPI revision number
	uint8_t chksum;				// Checksum of the table
	uint8_t oem_id[6];			// OEM ID string
	uint8_t oem_table_id[8];		// OEM Table ID string
	uint32_t oem_revision;			// OEM Revision
	uint32_t creator_id;			// Creator ID
	uint32_t creator_revision;		// Creator Revision
};

/* Extended System Description Table (XSDT) */
struct acpi_xsdt {
	struct acpi_header header;
	uint64_t entry[];
};

/* Root System Description Table (RSDT) */
struct acpi_rsdt {
	struct acpi_header header;
	uint32_t entry[];
};

/* Multiple APIC Description Table (MADT) */
struct acpi_madt {
	struct acpi_header header;
	uint32_t lapic_addr;
	uint32_t flags;
	uint8_t apic_structures[];
};

/* MADT Processor local APIC structure */
struct acpi_madt_lapic {
	uint8_t type;
	uint8_t length;
	uint8_t core_id;
	uint8_t lapic_id;
	uint32_t flags;
};

extern boolean_t _acpi_supported;

extern struct acpi_header *acpi_find_table(const char *signature);
extern void init_acpi();

#endif	/* __ACPI_H__ */
