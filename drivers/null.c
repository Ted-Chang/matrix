#include <types.h>
#include <stddef.h>
#include <errno.h>
#include "debug.h"
#include "fs.h"
#include "devfs.h"

int null_init(void)
{
	int rc = 0;
	struct vfs_node *n;
	devfs_handle_t h;

	n = vfs_lookup("/dev", VFS_DIRECTORY);
	if (!n) {
		rc = EGENERIC;
		DEBUG(DL_ERR, ("devfs not mounted.\n"));
		goto out;
	}

	rc = devfs_register((devfs_handle_t)n, "null", 0, NULL, NULL, &h);
	if (rc != 0 || h == INVALID_DEVFS_HANDLE) {
		DEBUG(DL_ERR, ("register device(null) failed, error:%x\n", rc));
		goto out;
	}

 out:
	if (rc != 0) {
		;
	}
	
	return rc;
}

int null_unload(void)
{
	int rc = 0;

	return rc;
}
