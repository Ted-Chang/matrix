#include <types.h>
#include <string.h>
#include "hal/hal.h"
#include "mm/malloc.h"
#include "dirent.h"
#include "initrd.h"
#include "debug.h"

static int initrd_mount(struct vfs_mount *mnt, size_t cnt);

struct vfs_type _ramfs_type = {
	.name = "ramfs",
	.desc = "Ramdisk file system",
	.ref_count = 0,
	.mount = initrd_mount,
};

struct initrd_header *initrd_hdr;
struct initrd_file_header *file_hdrs;
struct vfs_node *initrd_root;
struct vfs_node *initrd_dev;
struct vfs_node *root_nodes;

int nr_root_nodes;

struct dirent dirent;

static uint32_t initrd_read(struct vfs_node *node, uint32_t offset,
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
	if (node == initrd_root && index == 0) {
		strcpy(dirent.name, "dev");
		dirent.name[3] = 0;
		dirent.ino = 0;
		return &dirent;
	}

	if (index - 1 >= nr_root_nodes) {
		return 0;
	}

	strcpy(dirent.name, root_nodes[index - 1].name);
	dirent.name[strlen(root_nodes[index - 1].name)] = 0;
	dirent.ino = root_nodes[index - 1].inode;

	return &dirent;
}

static struct vfs_node *initrd_finddir(struct vfs_node *node, char *name)
{
	int i;

	if (node == initrd_root && !strcmp(name, "dev")) {
		return initrd_dev;
	}

	for (i = 0; i < nr_root_nodes; i++) {
		if (!strcmp(name, root_nodes[i].name))
			return &root_nodes[i];
	}

	return 0;
}

struct vfs_node *init_initrd(uint32_t location)
{
	int i;
	
	initrd_hdr = (struct initrd_header *)location;
	file_hdrs = (struct initrd_file_header *)
		(location + sizeof(struct initrd_header));

	/* Initialize the root directory */
	initrd_root = vfs_node_alloc(VFS_DIRECTORY);
	strcpy(initrd_root->name, "initrd");
	initrd_root->mask = initrd_root->uid =
		initrd_root->gid =
		initrd_root->inode =
		initrd_root->length =
		0;

	initrd_root->read = 0;
	initrd_root->write = 0;
	initrd_root->open = 0;
	initrd_root->close = 0;
	initrd_root->readdir = initrd_readdir;
	initrd_root->finddir = initrd_finddir;
	initrd_root->ptr = 0;
	initrd_root->impl = 0;

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
		root_nodes[i].read = initrd_read;
		root_nodes[i].write = 0;
		root_nodes[i].open = 0;
		root_nodes[i].close = 0;
		root_nodes[i].readdir = 0;
		root_nodes[i].finddir = 0;
		root_nodes[i].impl = 0;
	}

	return initrd_root;
}

int initrd_mount(struct vfs_mount *mnt, size_t cnt)
{
	int rc = -1;

	mnt->root = vfs_node_alloc(VFS_DIRECTORY);
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
