#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "fs.h"
#include "debug.h"

static int procfs_mount(struct vfs_mount *mnt, size_t cnt);

struct vfs_type _procfs_type = {
	.name = "procfs",
	.desc = "Process file system",
	.ref_count = 0,
	.mount = procfs_mount
};

int procfs_mount(struct vfs_mount *mnt, size_t cnt)
{
	int rc = -1;

	return rc;
}

static struct vfs_node_ops _procfs_node_ops = {
	.read = NULL,
	.write = NULL,
	.create = NULL,
	.close = NULL,
	.readdir = NULL,
	.finddir = NULL
};

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
