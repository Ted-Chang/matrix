#include <types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include "matrix/matrix.h"
#include "debug.h"
#include "mm/malloc.h"
#include "device.h"

int dev_create(int flags, void *ext, struct dev **dp)
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
	*dp = device;
	
	rc = 0;

 out:
	return rc;
}

int dev_open()
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

 out:
	return rc;
}

void dev_destroy(struct dev *d)
{
	ASSERT(d != NULL);
	kfree(d);
}

void init_dev()
{
	;
}
