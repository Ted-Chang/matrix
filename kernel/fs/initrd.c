#include <types.h>
#include <string.h>
#include "hal/hal.h"
#include "mm/malloc.h"
#include "dirent.h"
#include "initrd.h"
#include "debug.h"

extern struct vfs_node *vfs_node_alloc(struct vfs_mount *mnt, uint32_t type,
				       struct vfs_node_ops *ops, void *data);

static int initrd_mount(struct vfs_mount *mnt, size_t cnt);

struct vfs_type _ramfs_type = {
	.name = "ramfs",
	.desc = "Ramdisk file system",
	.ref_count = 0,
	.mount = initrd_mount,
};

struct initrd_header *initrd_hdr = NULL;
struct initrd_file_header *file_hdrs = NULL;
struct vfs_node *root_nodes = NULL;

int nr_root_nodes = 0;

struct dirent dirent;

static int initrd_create(struct vfs_node *parent, const char *name,
			 uint32_t type, struct vfs_node **np)
{
	int rc = -1;

	DEBUG(DL_DBG, ("create(%s), type(%d).\n", name, type));

	return rc;
}

static int initrd_close(struct vfs_node *node)
{
	int rc = -1;

	DEBUG(DL_DBG, ("close(%s).\n", node->name));

	return rc;
}

static int initrd_read(struct vfs_node *node, uint32_t offset,
		       uint32_t size, uint8_t *buffer)
{
	struct initrd_file_header hdr;

	hdr = file_hdrs[node->inode];
	
	if (offset > hdr.length) {
		DEBUG(DL_DBG, ("offset(%d), length(%d)\n", offset, hdr.length));
		return 0;
	}
	if (offset + size > hdr.length) {
		size = hdr.length - offset;
	}
	memcpy(buffer, (uint8_t *)(hdr.offset + offset), size);

	return size;
}

static struct dirent *initrd_readdir(struct vfs_node *node, uint32_t index)
{
	if (index - 1 >= nr_root_nodes) {
		return NULL;
	}

	strcpy(dirent.name, root_nodes[index - 1].name);
	dirent.name[strlen(root_nodes[index - 1].name)] = 0;
	dirent.ino = root_nodes[index - 1].inode;

	return &dirent;
}

static struct vfs_node *initrd_finddir(struct vfs_node *node, char *name)
{
	int i;

	for (i = 0; i < nr_root_nodes; i++) {
		if (!strcmp(name, root_nodes[i].name)) {
			return &root_nodes[i];
		}
	}

	DEBUG(DL_DBG, ("%s not found.\n", name));

	return 0;
}

static struct vfs_node_ops _ramfs_node_ops = {
	.read = initrd_read,
	.write = NULL,
	.create = initrd_create,
	.close = initrd_close,
	.readdir = initrd_readdir,
	.finddir = initrd_finddir
};

void init_initrd(uint32_t location)
{
	int i;
	
	initrd_hdr = (struct initrd_header *)location;
	file_hdrs = (struct initrd_file_header *)
		(location + sizeof(struct initrd_header));

	/* Initialize the file nodes in root directory */
	root_nodes = (struct vfs_node *)
		kmalloc(sizeof(struct vfs_node) * initrd_hdr->nr_files, 0);
	nr_root_nodes = initrd_hdr->nr_files;

	/* For each file in the root directory */
	for (i = 0; i < nr_root_nodes; i++) {
		file_hdrs[i].offset += location;

		/* Create a new file node */
		strcpy(root_nodes[i].name, (const char *)&file_hdrs[i].name);
		root_nodes[i].ref_count = 1;
		root_nodes[i].mask = root_nodes[i].uid = root_nodes[i].gid = 0;
		root_nodes[i].length = file_hdrs[i].length;
		root_nodes[i].inode = i;
		root_nodes[i].type = VFS_FILE;
		root_nodes[i].ops = &_ramfs_node_ops;
	}
}

int initrd_mount(struct vfs_mount *mnt, size_t cnt)
{
	int rc = -1;

	mnt->root = vfs_node_alloc(mnt, VFS_DIRECTORY, &_ramfs_node_ops, NULL);
	ASSERT(mnt->root != NULL);

	rc = 0;

	return rc;
}

int initrd_init(void)
{
	int rc;
	
	rc = vfs_type_register(&_ramfs_type);
	if (rc != 0) {
		DEBUG(DL_DBG, ("module initrd initialize failed.\n"));
	} else {
		DEBUG(DL_DBG, ("module initrd initialize successfully.\n"));
	}

	return rc;
}
