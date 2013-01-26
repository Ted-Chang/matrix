#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/matrix.h"
#include "debug.h"
#include "mm/malloc.h"
#include "fs.h"
#include "dirent.h"
#include "devfs.h"

#define MAX_DEVFS_NODES	64

struct devfs_node {
	char name[128];
	uint32_t ino;
};
static struct devfs_node _devfs_nodes[MAX_DEVFS_NODES];
static int _nr_devfs_nodes = 0;

static ino_t _next_ino = 1;

static ino_t id_alloc()
{
	return _next_ino++;
}

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
	struct devfs_node *dn;
	struct dirent *new_dentry = NULL;
	
	ASSERT(dentry != NULL);

	if (index >= _nr_devfs_nodes) {
		goto out;
	}

	new_dentry = (struct dirent *)kmalloc(sizeof(struct dirent), 0);
	if (!new_dentry) {
		DEBUG(DL_INF, ("Allocate dirent failed, node(%s), index(%d).\n",
			       n->name, index));
		goto out;
	}

	memset(new_dentry, 0, sizeof(struct dirent));

	dn = &_devfs_nodes[index];
	
	strncpy(new_dentry->d_name, dn->name, 128);
	new_dentry->d_ino = dn->ino;
	
	*dentry = new_dentry;
	rc = 0;

 out:
	return rc;
}

static int devfs_finddir(struct vfs_node *n, const char *name, ino_t *id)
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

static struct vfs_mount_ops _devfs_mount_ops = {
	.umount = NULL,
	.flush = NULL,
	.read_node = NULL,
};

int devfs_mount(struct vfs_mount *mnt, int flags, const void *data)
{
	int rc = -1;

	mnt->ops = &_devfs_mount_ops;
	mnt->root = vfs_node_alloc(mnt, VFS_DIRECTORY, &_devfs_node_ops, NULL);
	ASSERT(mnt->root != NULL);

	DEBUG(DL_DBG, ("devfs mounted, flags(%x).\n", flags));
	
	rc = 0;
	
	return rc;
}

int devfs_init(void)
{
	int rc = -1;

	memset(_devfs_nodes, 0, sizeof(struct devfs_node) * MAX_DEVFS_NODES);
	
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
	uint32_t type;
	struct devfs_node *dn;
	struct vfs_node *n, *dev;

	ASSERT(handle != NULL);

	n = (struct vfs_node *)dir;
	if (n->type != VFS_DIRECTORY) {
		DEBUG(DL_INF, ("register device on non-directory, node(%s:%d).\n",
			       n->name, n->type));
		goto out;
	}

	if (strcmp(n->mount->type->name, "devfs") != 0) {
		DEBUG(DL_INF, ("register device on non-devfs, node(%s), fstype(%s).\n",
			       n->name, n->mount->type->name));
		goto out;
	}

	if (_nr_devfs_nodes > MAX_DEVFS_NODES) {
		DEBUG(DL_INF, ("too much devices in devfs.\n"));
		goto out;
	}

	// TODO: type should be retrieved from parameter
	type = VFS_BLOCKDEVICE;
	dev = vfs_node_alloc(n->mount, type, ops, NULL);
	if (!dev) {
		DEBUG(DL_INF, ("allocate node failed.\n"));
		goto out;
	}
	
	strncpy(dev->name, name, 128);
	dev->ino = id_alloc();
	dev->mask = 0755;

	vfs_node_refer(dev);	// Reference the node

	/* Insert into our cache tree. Since we don't have read_node function
	 * so only registered device can be exported by devfs.
	 */
	avl_tree_insert(&n->mount->nodes, dev->ino, dev);

	dn = &_devfs_nodes[_nr_devfs_nodes];
	strncpy(dn->name, name, 128);
	dn->ino = dev->ino;
	_nr_devfs_nodes++;
	
	*handle = dev;
	rc = 0;

 out:
	return rc;
}

int devfs_unregister(devfs_handle_t handle)
{
	int rc = -1;
	struct vfs_node *n;

	if (handle == INVALID_HANDLE) {
		goto out;
	}

	n = (struct vfs_node *)handle;
	if (n->type == VFS_DIRECTORY) {
		DEBUG(DL_INF, ("unregister directory node(%s).\n", n->name));
		goto out;
	}

	if (strcmp(n->mount->type->name, "devfs") != 0) {
		DEBUG(DL_INF, ("unregister directory node(%s), fstype(%s).\n",
			       n->name, n->mount->type->name));
	}

	/* Remove from our cache tree */
	avl_tree_remove(&n->mount->nodes, n->ino);
	
	vfs_node_deref(n);	// Dereference the node

	rc = 0;

 out:
	return rc;
}
