#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/matrix.h"
#include "fs.h"
#include "dirent.h"
#include "debug.h"
#include "devfs.h"

static int devfs_mount(struct vfs_mount *mnt, int flags, const void *data);

struct vfs_type _devfs_type = {
	.name = "devfs",
	.desc = "Device file system",
	.ref_count = 0,
	.mount = devfs_mount,
};

static int devfs_readdir(struct vfs_node *n, uint32_t index, struct dirent **dentry)
{
	int rc = -1;
	
	ASSERT(dentry != NULL);
	
	return rc;
}

static int devfs_finddir(struct vfs_node *node, const char *name, ino_t *id)
{
	int rc = -1;

	ASSERT(id != NULL);

	return rc;
}

static struct vfs_node_ops _devfs_node_ops = {
	.read = NULL,
	.write = NULL,
	.create = NULL,
	.close = NULL,
	.readdir = devfs_readdir,
	.finddir = devfs_finddir,
};

static int devfs_read_node(struct vfs_mount *mnt, ino_t id, struct vfs_node **np)
{
	int rc = -1;

	ASSERT(np != NULL);

	return rc;
}

static struct vfs_mount_ops _devfs_mount_ops = {
	.umount = NULL,
	.flush = NULL,
	.read_node = devfs_read_node,
};

int devfs_mount(struct vfs_mount *mnt, int flags, const void *data)
{
	int rc = -1;

	mnt->ops = &_devfs_mount_ops;
	mnt->root = vfs_node_alloc(mnt, VFS_DIRECTORY, &_devfs_node_ops, NULL);
	ASSERT(mnt->root != NULL);

	rc = 0;
	
	DEBUG(DL_DBG, ("devfs mounted, flags(%x).\n", flags));
	
	return rc;
}

int devfs_init(void)
{
	int rc = -1;

	rc = vfs_type_register(&_devfs_type);
	if (rc != 0) {
		DEBUG(DL_DBG, ("module devfs initialize failed.\n"));
		goto out;
	} else {
		DEBUG(DL_DBG, ("module devfs initialize successfully.\n"));
	}

 out:
	return rc;
}

int devfs_register(devfs_handle_t dir, const char *name, int flags, void *ops,
		   void *info, devfs_handle_t *handle)
{
	int rc = -1;
	struct vfs_node *n;

	ASSERT(handle != NULL);

	n = (struct vfs_node *)dir;
	if (n->type != VFS_DIRECTORY) {
		DEBUG(DL_INF, ("register device on non-directory, node(%s:%d).\n",
			       n->name, n->type));
		goto out;
	}

	if (strcmp(n->mount->type->name, "devfs") != 0) {
		DEBUG(DL_INF, ("register device on non-devfs, fstype(%s).\n",
			       n->mount->type->name));
		goto out;
	}

 out:
	return rc;
}

int devfs_unregister(devfs_handle_t handle)
{
	int rc = -1;

	return rc;
}
