#include <types.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include "debug.h"
#include "fs.h"
#include "device.h"
#include "devfs.h"

#define NULL_MAJOR	1

struct dev_ops _null_ops;

int null_open()
{
	int rc = -1;

	return rc;
}

int null_close(struct dev *d)
{
	int rc = -1;

	return rc;
}

int null_read(struct dev *d, off_t off, size_t size, uint8_t *buf)
{
	int rc = -1;

	DEBUG(DL_DBG, ("off(%x) size(%x).\n", off, size));

	return rc;
}

int null_write(struct dev *d, off_t off, size_t size, uint8_t *buf)
{
	int rc = -1;

	DEBUG(DL_DBG, ("off(%x) size(%x).\n", off, size));

	return rc;
}

void null_destroy(struct dev *d)
{
	;
}

int null_init(void)
{
	int rc = 0;
	struct vfs_node *n = NULL;
	struct dev *d = NULL;
	dev_t devno;

	/* Register NULL device class */
	rc = dev_register(NULL_MAJOR, "null");
	if (rc != 0) {
		DEBUG(DL_ERR, ("register NULL device class failed.\n"));
		goto out;
	}

	/* Open the root of devfs */
	n = vfs_lookup("/dev", VFS_DIRECTORY);
	if (!n) {
		rc = EGENERIC;
		DEBUG(DL_ERR, ("devfs not mounted.\n"));
		goto out;
	}

	/* Major number of null is 1 */
	devno = MKDEV(NULL_MAJOR, 0);
	rc = dev_create(devno, DEV_CREATE, NULL, &d);
	if (rc != 0) {
		DEBUG(DL_ERR, ("create device failed.\n"));
		goto out;
	}

	/* Initialize the entry point of all requests */
	memset(&_null_ops, 0, sizeof(_null_ops));
	_null_ops.open = null_open;
	_null_ops.close = null_close;
	_null_ops.read = null_read;
	_null_ops.write = null_write;
	_null_ops.destroy = null_destroy;
	d->ops = &_null_ops;

	/* Register a node in devfs */
	rc = devfs_register((devfs_handle_t)n, "null", 0, NULL, devno);
	if (rc != 0) {
		DEBUG(DL_ERR, ("register device(null) failed, err:%x\n", rc));
		goto out;
	}

 out:
	if (n) {
		vfs_node_deref(n);
	}
	
	if (rc != 0) {
		if (devno != 0) {
			dev_destroy(devno);
		}
		dev_unregister(NULL_MAJOR);
	}
	
	return rc;
}

int null_unload(void)
{
	int rc = -1;

	return rc;
}
