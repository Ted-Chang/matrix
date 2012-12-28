#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "list.h"
#include "mutex.h"
#include "debug.h"

static struct list _pci_drivers = {
	.prev = &_pci_drivers,
	.next = &_pci_drivers
};
static struct mutex _pci_drivers_lock;

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

	return rc;
}
