#include <types.h>
#include <stddef.h>
#include <errno.h>
#include "debug.h"
#include "fs.h"
#include "devfs.h"

int null_open()
{
	int rc = 0;

	return rc;
}

int null_close(struct dev *d)
{
	int rc = 0;

	return rc;
}

int null_read(struct dev *d, off_t off, size_t size)
{
	int rc = 0;

	return rc;
}

int null_write(struct dev *d, off_t off, size_t size)
{
	int rc = 0;

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
	devfs_handle_t h;

	n = vfs_lookup("/dev", VFS_DIRECTORY);
	if (!n) {
		rc = EGENERIC;
		DEBUG(DL_ERR, ("devfs not mounted.\n"));
		goto out;
	}

	rc = devfs_register((devfs_handle_t)n, "null", 0, NULL, NULL, &h);
	if (rc != 0) {
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
