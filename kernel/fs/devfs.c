#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "fs.h"
#include "debug.h"

static int devfs_mount(struct vfs_mount *mnt, size_t cnt);

struct vfs_type _devfs_type = {
	.name = "devfs",
	.desc = "Device file system",
	.ref_count = 0,
	.mount = devfs_mount,
};

static int devfs_create(struct vfs_node *parent, const char *name,
			uint32_t type, struct vfs_node **np)
{
	int rc = -1;

	return rc;
}

static int devfs_close(struct vfs_node *n)
{
	int rc = -1;

	return rc;
}

static struct vfs_node_ops _devfs_node_ops = {
	.read = NULL,
	.write = NULL,
	.create = devfs_create,
	.close = devfs_close,
	.readdir = NULL,
	.finddir = NULL
};

int devfs_mount(struct vfs_mount *mnt, size_t cnt)
{
	int rc = -1;

	mnt->root = vfs_node_alloc(mnt, VFS_DIRECTORY, &_devfs_node_ops, NULL);
	ASSERT(mnt->root != NULL);

	rc = 0;

	return rc;
}

int devfs_init(void)
{
	int rc = 0;

	rc = vfs_type_register(&_devfs_type);
	if (rc != 0) {
		DEBUG(DL_DBG, ("module devfs initialize failed.\n"));
	} else {
		DEBUG(DL_DBG, ("module devfs initialize successfully.\n"));
	}
	
	return rc;
}
