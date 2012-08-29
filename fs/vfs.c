#include <stddef.h>
#include <string.h>
#include "fs.h"
#include "matrix/debug.h"
#include "mm/mmgr.h"

struct vfs_node *root_node = 0;

uint32_t vfs_read(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
	if (node->read != 0)
		return node->read(node, offset, size, buffer);
	else
		return 0;
}

uint32_t vfs_write(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer)
{
	if (node->write != 0)
		return node->read(node, offset, size, buffer);
	else
		return 0;
}

void vfs_open(struct vfs_node *node)
{
	if (node->open != 0)
		return node->open(node);
}

void vfs_close(struct vfs_node *node)
{
	if (node->close != 0)
		return node->close(node);
}

struct dirent *vfs_readdir(struct vfs_node *node, uint32_t index)
{
	if (((node->flags & 0x07) == VFS_DIRECTORY) && (node->readdir != 0)) {
		return node->readdir(node, index);
	} else
		return 0;
}

struct vfs_node *vfs_finddir(struct vfs_node *node, char *name)
{
	if (((node->flags & 0x07) == VFS_DIRECTORY) && (node->finddir != 0))
		return node->finddir(node, name);
	else
		return 0;
}

struct vfs_node *vfs_clone(struct vfs_node *src)
{
	struct vfs_node *node;
	
	if (!src)
		return NULL;

	node = (struct vfs_node *)kmalloc(sizeof(struct vfs_node));
	memcpy(node, src, sizeof(struct vfs_node));

	return node;
}
