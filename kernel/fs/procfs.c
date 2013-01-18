#include <types.h>
#include <stddef.h>
#include <string.h>
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
		DEBUG(DL_INF, ("Allocate dirent failed, node(%s), index(%d)",
			       node->name, index));
		goto out;
	}

	memset(new_dentry, 0, sizeof(struct dirent));
	strncpy(new_dentry->d_name, "cmdline", 128);
	new_dentry->d_ino = index;
	*dentry = new_dentry;
	rc = 0;

 out:
	return rc;
}

static struct vfs_node_ops _procfs_node_ops = {
	.read = NULL,
	.write = NULL,
	.create = NULL,
	.close = NULL,
	.readdir = procfs_readdir,
	.finddir = NULL,
};

int procfs_mount(struct vfs_mount *mnt, int flags, const void *data)
{
	int rc = -1;

	mnt->root = vfs_node_alloc(mnt, VFS_DIRECTORY, &_procfs_node_ops, NULL);
	if (mnt->root == NULL) {
		DEBUG(DL_WRN, ("failed to allocate procfs root node.\n"));
		goto out;
	}

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
