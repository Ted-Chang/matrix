#include <types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include "matrix/matrix.h"
#include "debug.h"
#include "mm/malloc.h"
#include "fs.h"
#include "dirent.h"
#include "devfs.h"

#define NR_MAX_DEVS	64

struct vfs_node *_devfs_nodes[NR_MAX_DEVS];

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

static int devfs_create(struct vfs_node *parent, const char *name,
			uint32_t type, struct vfs_node **np)
{
	int i, rc = -1;
	struct vfs_node *n;

	ASSERT(parent->type == VFS_DIRECTORY);

	/* We don't support create directory for device file system */
	if (!np || type == VFS_DIRECTORY) {
		rc = EINVAL;
		goto out;
	}

	DEBUG(DL_DBG, ("create(%s), type(%d).\n", name, type));

	for (i = 0; i < NR_MAX_DEVS; i++) {
		if (!_devfs_nodes[i]) {
			break;
		}
	}

	if (i >= NR_MAX_DEVS) {
		rc = EGENERIC;
		goto out;
	}
	
	n = vfs_node_alloc(parent->mount, type, parent->ops, NULL);
	if (!n) {
		rc = ENOMEM;
		goto out;
	}

	strncpy(n->name, name, 127);
	n->ino = id_alloc();
	n->length = 0;
	n->mask = 0755;

	/* Keep the pointer */
	_devfs_nodes[i] = n;

	vfs_node_refer(n);

	*np = n;
	rc = 0;

 out:
	return rc;
}

static int devfs_close(struct vfs_node *node)
{
	int rc = 0;

	DEBUG(DL_DBG, ("close(%s:%d).\n", node->name, node->ino));

	return rc;
}

static int devfs_read(struct vfs_node *node, uint32_t offset,
		      uint32_t size, uint8_t *buffer)
{
	int rc = -1;

	return rc;
}

static int devfs_write(struct vfs_node *node, uint32_t offset,
		       uint32_t size, uint8_t *buffer)
{
	int rc = -1;

	return rc;
}

static int devfs_readdir(struct vfs_node *n, uint32_t index,
			 struct dirent **dentry)
{
	int rc = -1;
	struct dirent *new_dentry = NULL;
	struct vfs_node *node;
	
	ASSERT(dentry != NULL);

	if (index >= NR_MAX_DEVS) {
		rc = EINVAL;
		DEBUG(DL_INF, ("index(%d) exceeds max number.\n", index));
		goto out;
	}

	node = _devfs_nodes[index];
	if (!node) {
		rc = EINVAL;
		DEBUG(DL_INF, ("node at index(%d) is null.\n", index));
		goto out;
	}

	new_dentry = (struct dirent *)kmalloc(sizeof(struct dirent), 0);
	if (!new_dentry) {
		rc = ENOMEM;
		DEBUG(DL_INF, ("Allocate dirent failed, node(%s), index(%d).\n",
			       n->name, index));
		goto out;
	}

	memset(new_dentry, 0, sizeof(struct dirent));
	strncpy(new_dentry->d_name, node->name, 128);
	new_dentry->d_ino = node->ino;

	*dentry = new_dentry;
	rc = 0;

 out:
	return rc;
}

static int devfs_finddir(struct vfs_node *n, const char *name, ino_t *id)
{
	int rc = -1, i;

	ASSERT((id != NULL) && (name != NULL));

	for (i = 0; i < NR_MAX_DEVS; i++) {
		if (!_devfs_nodes[i]) {
			continue;
		}
		
		if (strcmp(name, _devfs_nodes[i]->name) == 0) {
			*id = _devfs_nodes[i]->ino;
			rc = 0;
			break;
		}
	}

	return rc;
}

static struct vfs_node_ops _devfs_node_ops = {
	.read = devfs_read,
	.write = devfs_write,
	.create = devfs_create,
	.close = devfs_close,
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
	if (mnt->root == NULL) {
		rc = ENOMEM;
		goto out;
	}
	strncpy(mnt->root->name, "devfs-root", 128);
	mnt->root->ino = 0;

	/* Don't forget to reference the root node */
	vfs_node_refer(mnt->root);

	DEBUG(DL_DBG, ("devfs mounted, flags(%x).\n", flags));
	
	rc = 0;

 out:
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
	uint32_t type;
	struct vfs_node *n, *dev;

	ASSERT(handle != NULL);

	n = (struct vfs_node *)dir;
	if (n->type != VFS_DIRECTORY) {
		rc = EINVAL;
		DEBUG(DL_INF,
		      ("register device on non-directory, node(%s:%d).\n",
		       n->name, n->type));
		goto out;
	}

	if (strcmp(n->mount->type->name, "devfs") != 0) {
		rc = EINVAL;
		DEBUG(DL_INF,
		      ("register device on non-devfs, node(%s), fstype(%s).\n",
		       n->name, n->mount->type->name));
		goto out;
	}

	// TODO: type should be retrieved from parameter
	type = VFS_CHARDEVICE;
	dev = vfs_node_alloc(n->mount, type, ops, NULL);
	if (!dev) {
		rc = ENOMEM;
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
		rc = EINVAL;
		goto out;
	}

	n = (struct vfs_node *)handle;
	if (n->type == VFS_DIRECTORY) {
		rc = EINVAL;
		DEBUG(DL_INF, ("unregister directory node(%s).\n", n->name));
		goto out;
	}

	/* Check the File System type */
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
