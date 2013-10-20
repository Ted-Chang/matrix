#include <types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include "matrix/matrix.h"
#include "debug.h"
#include "mm/malloc.h"
#include "device.h"
#include "rtl/hashtable.h"

int dev_create(uint32_t major, int flags, void *ext, dev_t *dp)
{
	int rc = -1;
	dev_t devno;
	uint32_t minor;
	struct dev *device;

	if ((major == 0) || (dp == NULL)) {
		rc = EINVAL;
		goto out;
	}

	minor = 0;
	devno = MKDEV(major, minor);

	device = kmalloc(sizeof(*device), 0);
	if (!device) {
		rc = ENOMEM;
		goto out;
	}

	memset(device, 0, sizeof(*device));

	device->flags = flags;
	device->data = ext;
	device->ref_count = 1;	// Initial refcnt of the device is 1
	device->dev = devno;

	*dp = devno;
	
	rc = 0;

 out:
	return rc;
}

int dev_open(dev_t dev, struct dev **dp)
{
	int rc = -1;
	uint32_t major, minor;
	struct hashtable *ht = NULL;

	major = MAJOR(dev);
	minor = MINOR(dev);

	/* rc = hashtable_lookup(ht, &minor, dp); */
	/* if (rc != 0) { */
	/* 	rc = ENOENT; */
	/* } */

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

void dev_destroy(dev_t dev)
{
	ASSERT(dev != 0);
}

void init_dev()
{
	;
}
