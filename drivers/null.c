#include <types.h>
#include <stddef.h>
#include <errno.h>
#include "debug.h"
#include "fs.h"
#include "device.h"
#include "devfs.h"

#define NULL_MAJOR	1

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

int null_read(struct dev *d, off_t off, size_t size)
{
	int rc = -1;

	return rc;
}

int null_write(struct dev *d, off_t off, size_t size)
{
	int rc = -1;

	return rc;
}

void null_destroy(struct dev *d)
{
	;
}

int null_init(void)
{
	int rc = 0;
	struct vfs_node *n;
	struct dev *d;
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

	/* Register a node in devfs */
	rc = devfs_register((devfs_handle_t)n, "null", 0, NULL, devno);
	if (rc != 0) {
		DEBUG(DL_ERR, ("register device(null) failed, error:%x\n", rc));
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
	int rc = 0;

	return rc;
}
