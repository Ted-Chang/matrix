#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "list.h"
#include "mutex.h"
#include "debug.h"
#include "pci.h"

#define PCI_CFG_ADDR	0xCF8		// Configuration Address Register
#define PCI_CFG_DATA	0xCFC		// Configuration Data Register

/* Macro to generate a config address */
#define PCI_ADDR(bus, dev, func, reg) \
	((uint32_t)(((bus & 0xFF) << 16)) | ((dev & 0x1F) << 11) | \
	 ((func & 0x07) << 8) | (reg & 0xFC) | ((uint32_t)0x80000000))

static struct spinlock _pci_cfg_lock;

static struct list _pci_drivers = {
	.prev = &_pci_drivers,
	.next = &_pci_drivers
};
static struct mutex _pci_drivers_lock;

int platform_init_pci()
{
	int rc;
	
	spinlock_init(&_pci_cfg_lock, "pci-cfg-lock");
	
	outportdw(PCI_CFG_ADDR, 0x80000000);
	rc = inportdw(PCI_CFG_ADDR) != 0x80000000 ? -1 : 0;
	
	return rc;
}

uint8_t platform_pci_cfg_read8(uint8_t bus, uint8_t dev,
			       uint8_t func, uint8_t reg)
{
	uint8_t ret;

	spinlock_acquire(&_pci_cfg_lock);
	outportdw(PCI_CFG_ADDR, PCI_ADDR(bus, dev, func, reg));
	ret = inportb(PCI_CFG_DATA + (reg & 3));
	spinlock_release(&_pci_cfg_lock);

	return ret;
}

void platform_pci_cfg_write8(uint8_t bus, uint8_t dev, uint8_t func,
			     uint8_t reg, uint8_t val)
{
	spinlock_acquire(&_pci_cfg_lock);
	outportdw(PCI_CFG_ADDR, PCI_ADDR(bus, dev, func, reg));
	outportb(PCI_CFG_DATA + (reg & 3), val);
	spinlock_release(&_pci_cfg_lock);
}

uint16_t platform_pci_cfg_read16(uint8_t bus, uint8_t dev,
				 uint8_t func, uint8_t reg)
{
	uint16_t ret;

	spinlock_acquire(&_pci_cfg_lock);
	outportdw(PCI_CFG_ADDR, PCI_ADDR(bus, dev, func, reg));
	ret = inportw(PCI_CFG_DATA + (reg & 2));
	spinlock_release(&_pci_cfg_lock);
	
	return ret;
}

void platform_pci_cfg_write16(uint8_t bus, uint8_t dev, uint8_t func,
			      uint8_t reg, uint16_t val)
{
	spinlock_acquire(&_pci_cfg_lock);
	outportdw(PCI_CFG_ADDR, PCI_ADDR(bus, dev, func, reg));
	outportw(PCI_CFG_DATA + (reg & 2), val);
	spinlock_release(&_pci_cfg_lock);
}

static int pci_scan_dev(int id, int dev, int func, int indent)
{
	int rc;
	uint16_t ret;
	uint16_t vendor_id;
	uint16_t device_id;
	uint8_t base_class;
	uint8_t sub_class;
	uint8_t prog_iface;
	uint8_t revision;
	uint8_t dest;

	/* Check vendor ID to determine if device exists */
	ret = platform_pci_cfg_read16(id, dev, func, PCI_CFG_VENDOR_ID);
	if (ret == 0xFFFF) {
		rc = 0;
		goto out;
	}

	/* Retrieve the device information */
	vendor_id = pci_cfg_read16(PCI_CFG_VENDOR_ID);
	device_id = pci_cfg_read16(PCI_CFG_DEVICE_ID);
	base_class = pci_cfg_read8(PCI_CFG_BASE_CLASS);
	sub_class = pci_cfg_read8(PCI_CFG_SUB_CLASS);
	prog_iface = pci_cfg_read8(PCI_CFG_PIFACE);
	revision = pci_cfg_read8(PCI_CFG_REVISION);

	kprintf("pci: V%u (vendor:%x device:%x class:%x %x)\n",
		revision, vendor_id, device_id, base_class, sub_class);

	/* Check for a PCI-to-PCI bridge */
	if (base_class == 0x06 && sub_class == 0x04) {
		dest = pci_cfg_read8(0x19);
		kprintf("pci: device %d is a PCI-to-PCI bridge to %u\n",
			id, dest);
	}

	rc = 0;

 out:
	return rc;
}

static int pci_scan_bus(int id, int indent)
{
	int rc;
	int i, j;
	uint8_t ret;

	kprintf("pci: scanning bus %d for devices...\n", id);
	
	for (i = 0; i < 32; i++) {
		ret = platform_pci_cfg_read8(id, i, 0, PCI_CFG_HDR_TYPE);
		if (ret & 0x80) {
			/* Multifunction device */
			for (j = 0; j < 8; j++) {
				rc = pci_scan_dev(id, i, j, indent + 1);
				if (rc != 0) {
					DEBUG(DL_WRN, ("pci: scan device failed\n"));
				}
			}
		} else {
			rc = pci_scan_dev(id, i, 0, indent + 1);
			if (rc != 0) {
				DEBUG(DL_WRN, ("pci: scan device failed\n"));
			}
		}
	}

	return rc;
}

uint8_t pci_cfg_read8(uint8_t reg)
{
	uint8_t ret = 0;

	return ret;
}

uint16_t pci_cfg_read16(uint8_t reg)
{
	uint16_t ret = 0;

	return ret;
}

int pci_drivers_register()
{
	int rc = 0;

	return rc;
}

int pci_drivers_unregister()
{
	int rc = 0;

	return rc;
}

int pci_init(void)
{
	int rc = 0;

	mutex_init(&_pci_drivers_lock, "pci-mutex", 0);

	/* Detect PCI presence */
	rc = platform_init_pci();
	if (rc != 0) {
		DEBUG(DL_WRN, ("pci: PCI not present or not usable\n"));
		goto out;
	}

	/* Scan the main bus */
	rc = pci_scan_bus(0, 0);

 out:
	return rc;
}
