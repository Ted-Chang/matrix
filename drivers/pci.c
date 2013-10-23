#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "list.h"
#include "mutex.h"
#include "debug.h"
#include "mm/malloc.h"
#include "device.h"
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
static uint32_t _pci_instance;

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

uint32_t platform_pci_cfg_read32(uint8_t bus, uint8_t dev,
				 uint8_t func, uint8_t reg)
{
	uint32_t ret;

	spinlock_acquire(&_pci_cfg_lock);
	outportdw(PCI_CFG_ADDR, PCI_ADDR(bus, dev, func, reg));
	ret = inportdw(PCI_CFG_DATA);
	spinlock_release(&_pci_cfg_lock);

	return ret;
}

void platform_pci_cfg_write32(uint8_t bus, uint8_t dev, uint8_t func,
			      uint8_t reg, uint32_t val)
{
	spinlock_acquire(&_pci_cfg_lock);
	outportdw(PCI_CFG_ADDR, PCI_ADDR(bus, dev, func, reg));
	outportdw(PCI_CFG_DATA, val);
	spinlock_release(&_pci_cfg_lock);
}

static int pci_scan_dev(int id, int dev, int func, int indent)
{
	int rc;
	uint16_t ret;
	struct dev *d;
	struct pci_dev *device;
	uint8_t dest;
	dev_t devno;

	/* Check vendor ID to determine if device exists */
	ret = platform_pci_cfg_read16(id, dev, func, PCI_CFG_VENDOR_ID);
	if (ret == 0xFFFF) {
		rc = 0;
		goto out;
	}

	device = (struct pci_dev *)kmalloc(sizeof(*device), 0);
	if (!device) {
		rc = 0;
		DEBUG(DL_DBG, ("kmalloc pci device failed.\n"));
		goto out;
	}

	LIST_INIT(&device->link);
	device->bus = (uint8_t)id;
	device->dev = (uint8_t)dev;
	device->func = (uint8_t)func;

	/* Retrieve the device information */
	device->vendor_id = pci_cfg_read16(device, PCI_CFG_VENDOR_ID);
	device->dev_id = pci_cfg_read16(device, PCI_CFG_DEVICE_ID);
	device->base_class = pci_cfg_read8(device, PCI_CFG_BASE_CLASS);
	device->sub_class = pci_cfg_read8(device, PCI_CFG_SUB_CLASS);
	device->prog_iface = pci_cfg_read8(device, PCI_CFG_PIFACE);
	device->revision = pci_cfg_read8(device, PCI_CFG_REVISION);
	device->cache_line_size = pci_cfg_read8(device, PCI_CFG_CACHE_LINE_SIZE);
	device->hdr_type = pci_cfg_read8(device, PCI_CFG_HDR_TYPE);
	device->subsys_vendor = pci_cfg_read16(device, PCI_CFG_SUBSYS_VENDOR);
	device->subsys_id = pci_cfg_read16(device, PCI_CFG_SUBSYS_ID);
	device->int_line = pci_cfg_read8(device, PCI_CFG_INT_LINE);
	device->int_pin = pci_cfg_read8(device, PCI_CFG_INT_PIN);

	/* Create a device for it */
	devno = MKDEV(PCI_MAJOR, _pci_instance++);
	rc = dev_create(devno, DEV_CREATE, NULL, &d);
	if (rc != 0) {
		goto out;
	}

	kprintf("pci: V%u (vendor:%x device:%x class:%x %x)\n",
		device->revision, device->vendor_id, device->dev_id,
		device->base_class, device->sub_class);

	/* Check for a PCI-to-PCI bridge */
	if (device->base_class == 0x06 && device->sub_class == 0x04) {
		dest = pci_cfg_read8(device, 0x19);
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
	struct dev *d;
	dev_t devno;

	/* Create the bus device */
	devno = MKDEV(PCI_MAJOR, _pci_instance++);
	rc = dev_create(devno, DEV_CREATE, NULL, &d);
	if (rc != 0) {
		goto out;
	}

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

 out:
	return rc;
}

uint8_t pci_cfg_read8(struct pci_dev *dev, uint8_t reg)
{
	return platform_pci_cfg_read8(dev->bus, dev->dev, dev->func, reg);
}

void pci_cfg_write8(struct pci_dev *dev, uint8_t reg, uint8_t val)
{
	platform_pci_cfg_write8(dev->bus, dev->dev, dev->func, reg, val);
}

uint16_t pci_cfg_read16(struct pci_dev *dev, uint8_t reg)
{
	return platform_pci_cfg_read16(dev->bus, dev->dev, dev->func, reg);
}

void pci_cfg_write16(struct pci_dev *dev, uint8_t reg, uint16_t val)
{
	platform_pci_cfg_write16(dev->bus, dev->dev, dev->func, reg, val);
}

uint32_t pci_cfg_read32(struct pci_dev *dev, uint8_t reg)
{
	return platform_pci_cfg_read32(dev->bus, dev->dev, dev->func, reg);
}

void pci_cfg_write32(struct pci_dev *dev, uint8_t reg, uint32_t val)
{
	platform_pci_cfg_write32(dev->bus, dev->dev, dev->func, reg, val);
}

int pci_driver_register()
{
	int rc = 0;

	return rc;
}

int pci_driver_unregister()
{
	int rc = 0;

	ASSERT(0);	// TODO: Unregister driver

	return rc;
}

int pci_init(void)
{
	int rc = 0;

	/* Initial instance count is 0 */
	_pci_instance = 0;
	
	/* Register PCI device class */
	rc = dev_register(PCI_MAJOR, "pci");
	if (rc != 0) {
		DEBUG(DL_WRN, ("pci: register PCI device class failed.\n"));
		goto out;
	}

	mutex_init(&_pci_drivers_lock, "pci-mutex", 0);

	/* Detect PCI presence */
	rc = platform_init_pci();
	if (rc != 0) {
		DEBUG(DL_WRN, ("pci: PCI not present or not usable.\n"));
		goto out;
	}

	/* Scan the main bus */
	rc = pci_scan_bus(0, 0);

 out:
	return rc;
}

int pci_unload(void)
{
	int rc = -1;

	return rc;
}
