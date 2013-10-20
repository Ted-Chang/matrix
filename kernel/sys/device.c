#include <types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include "matrix/matrix.h"
#include "debug.h"
#include "mm/malloc.h"
#include "device.h"

int dev_create(uint16_t major, int flags, void *ext, dev_t *dp)
{
	int rc = -1;
	struct dev *device;

	device = kmalloc(sizeof(*device), 0);
	if (!device) {
		rc = ENOMEM;
		goto out;
	}

	memset(device, 0, sizeof(*device));

	device->flags = flags;
	device->data = ext;
	device->ref_count = 1;	// Initial refcnt of the device is 1

	// TODO: generate a device ID
	
	rc = 0;

 out:
	return rc;
}

int dev_open(dev_t dev_id, struct dev **dp)
{
	int rc = -1;

	return rc;
}

int dev_close(struct dev *d)
{
	int rc = -1;

	if (!d) {
		rc = EINVAL;
		goto out;
	}

	if (!d->ops || !d->ops->close) {
		rc = EGENERIC;
		goto out;
	}
	
	rc = d->ops->close(d);

 out:
	return rc;
}

int dev_read(struct dev *d, off_t off, size_t size)
{
	int rc = -1;

	if (!d) {
		rc = EINVAL;
		goto out;
	}

	if (!d->ops || !d->ops->read) {
		rc = EGENERIC;
		goto out;
	}

	rc = d->ops->read(d, off, size);

 out:
	return rc;
}

int dev_write(struct dev *d, off_t off, size_t size)
{
	int rc = -1;

	if (!d) {
		rc = EINVAL;
		goto out;
	}

	if (!d->ops || !d->ops->write) {
		rc = EGENERIC;
		goto out;
	}

	rc = d->ops->write(d, off, size);

 out:
	return rc;
}

void dev_destroy(dev_t dev_id)
{
	ASSERT(dev_id != 0);
}

void init_dev()
{
	;
}
