#ifndef __PCI_H__
#define __PCI_H__

#include "list.h"

#define PCI_MAJOR	3

#define PCI_CFG_VENDOR_ID	0x00	// Vendor ID
#define PCI_CFG_DEVICE_ID	0x02	// Device ID
#define PCI_CFG_COMMAND		0x04	// Command
#define PCI_CFG_STATUS		0x06	// Status
#define PCI_CFG_REVISION	0x08	// Revision
#define PCI_CFG_PIFACE		0x09	// Program Interface
#define PCI_CFG_SUB_CLASS	0x0A	// Sub class
#define PCI_CFG_BASE_CLASS	0x0B	// Base class
#define PCI_CFG_CACHE_LINE_SIZE	0x0C	// Cache line size
#define PCI_CFG_LATENCY		0x0D	// Latency timer
#define PCI_CFG_HDR_TYPE	0x0E	// Header type
#define PCI_CFG_BIST		0x0F	// BIST
#define PCI_CFG_BAR0		0x10	// BAR0
#define PCI_CFG_BAR1		0x14	// BAR1
#define PCI_CFG_BAR2		0x18	// BAR2
#define PCI_CFG_BAR3		0x1C	// BAR3
#define PCI_CFG_BAR4		0x20	// BAR4
#define PCI_CFG_BAR5		0x24	// BAR5
#define PCI_CFG_CARDBUS_CIS	0x28	// Cardbus CIS Ptr
#define PCI_CFG_SUBSYS_VENDOR	0x2C	// Subsystem vendor
#define PCI_CFG_SUBSYS_ID	0x2E	// Subsystem ID
#define PCI_CFG_ROM_ADDR	0x30	// ROM base address
#define PCI_CFG_INT_LINE	0x3C	// Interrupt line
#define PCI_CFG_INT_PIN		0x3D	// Interrupt pin
#define PCI_CFG_MIN_GRANT	0x3E	// Min grant
#define PCI_CFG_MAX_LATENCY	0x3F	// Max latency

struct dev;

struct pci_dev {
	struct list link;
	dev_t devno;			// Device ID

	/* Location of the device */
	uint8_t bus;			// Bus ID
	uint8_t dev;			// Device number
	uint8_t func;			// Function number

	/* Information about the device */
	uint16_t vendor_id;		// Vendor ID
	uint16_t dev_id;		// Device ID
	uint8_t base_class;		// Base class ID
	uint8_t sub_class;		// Sub class ID
	uint8_t prog_iface;		// Programming interface
	uint8_t revision;		// Revision
	uint8_t cache_line_size;	// Cache line size (number of DWORDs)
	uint8_t hdr_type;		// Header type
	uint16_t subsys_vendor;		// Subsystem vendor
	uint16_t subsys_id;		// Subsystem ID
	uint8_t int_line;		// Interrupt line
	uint8_t int_pin;		// Interrupt pin
};

extern uint8_t pci_cfg_read8(struct pci_dev *dev, uint8_t reg);
extern void pci_cfg_write8(struct pci_dev *dev, uint8_t reg, uint8_t val);
extern uint16_t pci_cfg_read16(struct pci_dev *dev, uint8_t reg);
extern void pci_cfg_write16(struct pci_dev *dev, uint8_t reg, uint16_t val);
extern uint32_t pci_cfg_read32(struct pci_dev *dev, uint8_t reg);
extern void pci_cfg_write32(struct pci_dev *dev, uint8_t reg, uint32_t val);

#endif	/* __PCI_H__ */
