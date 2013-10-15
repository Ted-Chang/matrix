#include <types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include "matrix/matrix.h"
#include "mm/malloc.h"
#include "fs.h"
#include "dirent.h"
#include "debug.h"

int _nr_procfs_nodes = 1;

static int procfs_mount(struct vfs_mount *mnt, int flags, const void *data);

struct vfs_type _procfs_type = {
	.name = "procfs",
	.desc = "Process file system",
	.ref_count = 0,
	.mount = procfs_mount
};

static int procfs_create(struct vfs_node *parent, const char *name,
			 uint32_t type, struct vfs_node **np)
{
	int rc = -1;

	DEBUG(DL_DBG, ("create(%s), type(%d).\n", name, type));

	return rc;
}

static int procfs_close(struct vfs_node *node)
{
	int rc = 0;

	DEBUG(DL_DBG, ("close(%s:%d)\n", node->name, node->ino));

	return rc;
}

static int procfs_readdir(struct vfs_node *node, uint32_t index, struct dirent **dentry)
{
	int rc = -1;
	struct dirent *new_dentry = NULL;

	ASSERT(dentry != NULL);
		
	if (index >= _nr_procfs_nodes) {
		goto out;
	}

	new_dentry = kmalloc(sizeof(struct dirent), 0);
	if (!new_dentry) {
		DEBUG(DL_INF, ("Allocate dirent failed, node(%s), index(%d).\n",
			       node->name, index));
		goto out;
	}

	memset(new_dentry, 0, sizeof(struct dirent));
	strncpy(new_dentry->d_name, "cmdline", 128);
	new_dentry->d_ino = 1;
	*dentry = new_dentry;
	rc = 0;

 out:
	return rc;
}

static int procfs_finddir(struct vfs_node *node, const char *name, ino_t *id)
{
	int rc = -1;

	ASSERT(id != NULL);

	return rc;
}

static struct vfs_node_ops _procfs_node_ops = {
	.read = NULL,
	.write = NULL,
	.create = procfs_create,
	.close = procfs_close,
	.readdir = procfs_readdir,
	.finddir = procfs_finddir,
};

static int procfs_read_node(struct vfs_mount *mnt, ino_t id, struct vfs_node **np)
{
	int rc = -1;

	ASSERT(np != NULL);

	return rc;
}

static struct vfs_mount_ops _procfs_mount_ops = {
	.umount = NULL,
	.flush = NULL,
	.read_node = procfs_read_node,
};

int procfs_mount(struct vfs_mount *mnt, int flags, const void *data)
{
	int rc = -1;

	mnt->ops = &_procfs_mount_ops;
	mnt->root = vfs_node_alloc(mnt, VFS_DIRECTORY, &_procfs_node_ops, NULL);
	if (mnt->root == NULL) {
		rc = ENOMEM;
		DEBUG(DL_WRN, ("failed to allocate procfs root node.\n"));
		goto out;
	}
	strncpy(mnt->root->name, "procfs-root", 128);
	mnt->root->ino = 0;

	/* Don't forget to reference the root node */
	vfs_node_refer(mnt->root);

	rc = 0;

	DEBUG(DL_DBG, ("procfs mounted, flags(%x).\n", flags));

 out:
	return rc;
}

int procfs_init(void)
{
	int rc = -1;

	rc = vfs_type_register(&_procfs_type);
	if (rc != 0) {
		DEBUG(DL_DBG, ("module procfs initialize failed.\n"));
	} else {
		DEBUG(DL_DBG, ("module procfs initialize successfully.\n"));
	}

	return rc;
}

int procfs_unload(void)
{
	int rc = -1;

	return rc;
}
