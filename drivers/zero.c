#include <types.h>
#include <stddef.h>
#include <errno.h>
#include "debug.h"
#include "fs.h"
#include "device.h"
#include "devfs.h"

#define ZERO_MAJOR	2

int zero_init()
{
	int rc = 0;
	struct vfs_node *n = NULL;
	struct dev *d = NULL;
	dev_t devno;

	/* Register ZERO device class */
	rc = dev_register(ZERO_MAJOR, "zero");
	if (rc != 0) {
		DEBUG(DL_ERR, ("register ZERO device class failed.\n"));
		goto out;
	}

	/* Open the root of devfs */
	n = vfs_lookup("/dev", VFS_DIRECTORY);
	if (!n) {
		rc = EGENERIC;
		DEBUG(DL_ERR, ("devfs not mounted.\n"));
		goto out;
	}

	/* Major number of zero is 2 */
	devno = MKDEV(ZERO_MAJOR, 0);
	rc = dev_create(devno, DEV_CREATE, NULL, &d);
	if (rc != 0) {
		DEBUG(DL_ERR, ("create device failed.\n"));
		goto out;
	}

	/* Register a node in devfs */
	rc = devfs_register((devfs_handle_t)n, "zero", 0, NULL, devno);
	if (rc != 0) {
		DEBUG(DL_ERR, ("register device(zero) failed, err:%x\n", rc));
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
		dev_unregister(ZERO_MAJOR);
	}

	return rc;
}

int zero_unload(void)
{
	int rc = -1;

	return rc;
}
