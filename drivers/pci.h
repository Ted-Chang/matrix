#ifndef __PCI_H__
#define __PCI_H__

#include "list.h"

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

struct pci_dev {
	struct list link;

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

#endif
