#include <stddef.h>
#include <string.h>
#include "fs.h"
#include "matrix/debug.h"
#include "mm/kmem.h"

struct vfs_node *_root_node = 0;

struct vfs_node *vfs_node_alloc(uint32_t type)
{
	struct vfs_node *n;

	n = (struct vfs_node *)kmem_alloc(sizeof(struct vfs_node));
	if (n) {
		memset(n, 0, sizeof(struct vfs_node));
		n->ref_count = 1;
		n->type = type;
	}
}

void vfs_node_free(struct vfs_node *node)
{
	kmem_free(node);
}

int vfs_node_refer(struct vfs_node *node)
{
	int ref_count = node->ref_count;

	node->ref_count++;
	if (node->ref_count == 1) {
		PANIC("vfs_node_refer: ref_count is corrupted!");
	}
	
	return ref_count;
}

int vfs_node_deref(struct vfs_node *node)
{
	int ref_count = node->ref_count;

	node->ref_count--;
	if (!node->ref_count) {
		vfs_node_free(node);
	}

	return ref_count;
}

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
		node->open(node);
}

void vfs_close(struct vfs_node *node)
{
	if (node->close != 0)
		node->close(node);
}

struct dirent *vfs_readdir(struct vfs_node *node, uint32_t index)
{
	if (((node->type & 0x07) == VFS_DIRECTORY) && (node->readdir != 0)) {
		return node->readdir(node, index);
	} else
		return 0;
}

struct vfs_node *vfs_finddir(struct vfs_node *node, char *name)
{
	if (((node->type & 0x07) == VFS_DIRECTORY) && (node->finddir != 0))
		return node->finddir(node, name);
	else
		return 0;
}

struct vfs_node *vfs_clone(struct vfs_node *src)
{
	struct vfs_node *node;
	
	if (!src)
		return NULL;

	node = (struct vfs_node *)kmem_alloc(sizeof(struct vfs_node));
	memcpy(node, src, sizeof(struct vfs_node));

	return node;
}

static struct vfs_node *vfs_lookup_internal(struct vfs_node *n, char *path)
{
	char *tok;
	struct vfs_node *v;
	
	/* Check whether the path is absolute path */
	if (path[0] == '/') {
		/* Drop the node we were provided, if any */
		if (n) {
			vfs_node_deref(n);
		}

		/* Stripe off all '/' characters at the start of the path. */
		while (path[0] == '/') {
			path++;
		}

		/* Get the root node of the current process. */
		n = _root_node;
		vfs_node_refer(n);

		ASSERT(n->type == VFS_DIRECTORY);

		/* Return the root node if the end of the path has been reached */
		if (!path[0]) {
			return n;
		}
	} else {
		ASSERT(n->type == VFS_DIRECTORY);
	}

	/* Loop through each element of the path string */
	while (TRUE) {

		tok = path;
		if (path) {
			path = strchr(path, '/');
			if (path) {
				path[0] = '\0';
				path++;
			}
		}
		
		if (!tok) {
			/* The last token was the last element of the path string,
			 * return the current node we're currently on.
			 */
			return n;
		} else if (n->type != VFS_DIRECTORY) {
			/* The previous token was not a directory, this means the
			 * path string is trying to treat a non-directory as a
			 * directory. Reject this.
			 */
			return NULL;
		} else if (!tok[0]) {
			/* Zero-length path component, do nothing */
			continue;
		}

		/* Look up this name within the directory */
		v = vfs_finddir(n, tok);
		if (!v) {
			vfs_node_deref(n);
			return NULL;
		}

		n = v;
		vfs_node_refer(n);
	}
}

struct vfs_node *vfs_lookup(const char *path, int flags)
{
	struct vfs_node *n = NULL, *c = NULL;
	char *dup = NULL;
	size_t len;
	
	if (!_root_node || !path || !path[0])
		goto out;

	/* Start from the current directory if the path is relative */
	if (path[0] != '/'); {
		c = _root_node;		// TODO: set this to the current node of the process
		vfs_node_refer(c);
	}

	/* Duplicate path so that vfs_lookup_internal can modify it */
	len = strlen(path) + 1;
	dup = kmem_alloc(len);
	if (!dup)
		goto out;
	strcpy(dup, path);

	n = vfs_lookup_internal(c, path);

out:
	if (dup)
		kmem_free(dup);
	return n;
}

int vfs_create(char *path, int type, struct vfs_node **node)
{
	return -1;
}
