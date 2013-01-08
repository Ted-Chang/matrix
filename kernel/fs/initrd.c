#include <types.h>
#include <string.h>
#include "hal/hal.h"
#include "mm/malloc.h"
#include "dirent.h"
#include "initrd.h"
#include "debug.h"

struct ramfs_node {
	char name[128];
	uint32_t type;
	uint32_t inode;
	uint32_t length;
};

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
struct ramfs_node *_initrd_nodes = NULL;

int _nr_initrd_nodes = 0;
int _nr_total_initrd_nodes = 0;

struct dirent dirent;

static int initrd_create(struct vfs_node *parent, const char *name,
			 uint32_t type, struct vfs_node **np)
{
	int rc = -1;
	struct vfs_node *n;

	DEBUG(DL_DBG, ("create(%s), type(%d).\n", name, type));

	if (type != VFS_DIRECTORY) {	// Only support create directory now
		goto out;
	}

	if (_nr_initrd_nodes >= _nr_total_initrd_nodes) {
		goto out;
	}

	strcpy(_initrd_nodes[_nr_initrd_nodes].name, name);
	_initrd_nodes[_nr_initrd_nodes].inode = _nr_initrd_nodes;
	_initrd_nodes[_nr_initrd_nodes].type = type;
	_initrd_nodes[_nr_initrd_nodes].length = 0;
	_nr_initrd_nodes++;

	rc = 0;

 out:
	return rc;
}

static int initrd_close(struct vfs_node *node)
{
	int rc = 0;

	DEBUG(DL_DBG, ("close(%s).\n", node->name));

	return rc;
}

static int initrd_read(struct vfs_node *node, uint32_t offset,
		       uint32_t size, uint8_t *buffer)
{
	struct initrd_file_header *hdr;

	hdr = &file_hdrs[node->inode];
	
	if (offset > hdr->length) {
		DEBUG(DL_DBG, ("offset(%d), length(%d)\n", offset, hdr->length));
		return 0;
	}
	if (offset + size > hdr->length) {
		size = hdr->length - offset;
	}
	memcpy(buffer, (uint8_t *)(hdr->offset + offset), size);

	return size;
}

static struct dirent *initrd_readdir(struct vfs_node *node, uint32_t index)
{
	if (index - 1 >= _nr_initrd_nodes) {
		return NULL;
	}

	strcpy(dirent.name, _initrd_nodes[index - 1].name);
	dirent.name[strlen(_initrd_nodes[index - 1].name)] = 0;
	dirent.ino = _initrd_nodes[index - 1].inode;

	return &dirent;
}

static struct vfs_node *initrd_finddir(struct vfs_node *node, char *name)
{
	int i;
	struct vfs_node *n;

	n = NULL;
	
	for (i = 0; i < _nr_initrd_nodes; i++) {
		if (!strcmp(name, _initrd_nodes[i].name)) {
			n = vfs_node_alloc(node->mount, _initrd_nodes[i].type, node->ops, NULL);
			n->length = _initrd_nodes[i].length;
			n->inode = _initrd_nodes[i].inode;
			break;
		}
	}

	if (!n) {
		DEBUG(DL_DBG, ("%s not found.\n", name));
	}

	return n;
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
	size_t size;
	
	initrd_hdr = (struct initrd_header *)location;
	file_hdrs = (struct initrd_file_header *)(location + sizeof(struct initrd_header));

	/* Initialize the file nodes in root directory, we will allocate 10 more
	 * nodes for creating new nodes.
	 */
	_nr_total_initrd_nodes = initrd_hdr->nr_files + 10;
	_nr_initrd_nodes = initrd_hdr->nr_files;

	size = sizeof(struct ramfs_node) * (_nr_total_initrd_nodes);
	_initrd_nodes = (struct ramfs_node *)kmalloc(size, 0);
	ASSERT(_initrd_nodes != NULL);
	
	memset(_initrd_nodes, 0, size);
	
	/* For each file in the root directory initialize the corresponding ramfs
	 * nodes.
	 */
	for (i = 0; i < _nr_initrd_nodes; i++) {
		file_hdrs[i].offset += location;

		/* Create a new file node */
		strcpy(_initrd_nodes[i].name, file_hdrs[i].name);
		_initrd_nodes[i].inode = i;
		_initrd_nodes[i].type = VFS_FILE;
		_initrd_nodes[i].length = file_hdrs[i].length;
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
