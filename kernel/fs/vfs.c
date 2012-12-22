#include <stddef.h>
#include <string.h>
#include "fs.h"
#include "debug.h"
#include "mm/malloc.h"
#include "mm/slab.h"
#include "mutex.h"
#include "proc/process.h"

/* List of registered File Systems */
static struct list _fs_list = {
	.prev = &_fs_list,
	.next = &_fs_list
};
static struct mutex _fs_list_lock;

/* List of all mounts */
static struct list _mount_list = {
	.prev = &_mount_list,
	.next = &_mount_list
};
static struct mutex _mount_list_lock;

/* Cache of Virtual File System node structure */
static slab_cache_t _vfs_node_cache;

/* Mount at the root of the File System */
struct vfs_mount *_root_mount = NULL;

struct vfs_node *_root_node = 0;

struct vfs_node *vfs_node_alloc(uint32_t type)
{
	struct vfs_node *n;

	n = (struct vfs_node *)slab_cache_alloc(&_vfs_node_cache);
	if (n) {
		memset(n, 0, sizeof(struct vfs_node));
		n->ref_count = 1;
		n->type = type;
	}

	return n;
}

void vfs_node_free(struct vfs_node *node)
{
	slab_cache_free(&_vfs_node_cache, node);
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
	
	if (!src) {
		return NULL;
	}

	node = (struct vfs_node *)kmalloc(sizeof(struct vfs_node), 0);
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
			DEBUG(DL_DBG, ("component(%s) type(0x%x)\n",
				       n->name, n->type));
			return NULL;
		} else if (!tok[0]) {
			/* Zero-length path component, do nothing */
			continue;
		}

		/* Look up this name within the directory */
		v = vfs_finddir(n, tok);
		if (!v) {
			DEBUG(DL_DBG, ("%s not found.\n", tok));
			vfs_node_deref(n);
			return NULL;
		}

		n = v;
		vfs_node_refer(n);
	}
}

struct vfs_node *vfs_lookup(const char *path, int type)
{
	struct vfs_node *n = NULL, *c = NULL;
	char *dup = NULL;
	size_t len;
	
	if (!_root_node || !path || !path[0]) {
		goto out;
	}

	/* Start from the current directory if the path is relative */
	if (path[0] != '/'); {
		c = _root_node;		// TODO: set this to the current node of the process
		vfs_node_refer(c);
	}

	/* Duplicate path so that vfs_lookup_internal can modify it */
	len = strlen(path) + 1;
	dup = (char *)kmalloc(len, 0);
	if (!dup) {
		goto out;
	}
	strcpy(dup, path);

	/* Look up the path string */
	n = vfs_lookup_internal(c, dup);
	if (n) {
		if ((type >= 0) && (n->type != type)) {
			vfs_node_deref(n);
			n = NULL;
		}
	}

out:
	if (dup) {
		kfree(dup);
	}
	return n;
}

int vfs_create(const char *path, int type, struct vfs_node **node)
{
	return -1;
}

static struct vfs_type *vfs_type_lookup_internal(const char *name)
{
	struct list *l;
	struct vfs_type *type;
	
	LIST_FOR_EACH(l, &_fs_list) {
		type = LIST_ENTRY(l, struct vfs_type, link);
		if (strcmp(type->name, name) == 0) {
			return type;
		}
	}

	return NULL;
}

static struct vfs_type *vfs_type_lookup(const char *name)
{
	struct vfs_type *type;

	mutex_acquire(&_fs_list_lock);

	type = vfs_type_lookup_internal(name);
	if (type) {
		atomic_inc(&type->ref_count);
	}

	mutex_release(&_fs_list_lock);

	return type;
}

int vfs_type_register(struct vfs_type *type)
{
	int rc = -1;
	
	/* Check whether the structure is valid */
	if (!type || !type->name || !type->desc) {
		return rc;
	}

	mutex_acquire(&_fs_list_lock);

	/* Check whether this File System has been registered */
	if (NULL != vfs_type_lookup_internal(type->name)) {
		goto out;
	}
	
	type->ref_count = 0;
	list_add_tail(&type->link, &_fs_list);

	rc = 0;
	
	DEBUG(DL_DBG, ("registered file system(%s).\n", type->name));

 out:
	mutex_release(&_fs_list_lock);

	return rc;
}

int vfs_type_unregister(struct vfs_type *type)
{
	int rc = -1;
	
	mutex_acquire(&_fs_list_lock);

	if (vfs_type_lookup_internal(type->name) != type) {
		;
	} else if (type->ref_count > 0) {
		;
	} else {
		ASSERT(type->ref_count == 0);
		list_del(&type->link);
		rc = 0;
	}

	mutex_release(&_fs_list_lock);

	return rc;
}

int vfs_mount(const char *path, const char *type, const char *opts)
{
	int rc = -1, flags = 0;
	struct vfs_node *n;
	struct vfs_mount *mnt = NULL;

	if (!path) {
		return rc;
	}

	mutex_acquire(&_mount_list_lock);

	/* If the root File System is not mounted yet, the only place we can
	 * mount is root
	 */
	if (!_root_mount) {
		ASSERT(CURR_PROC == _kernel_proc);
		if (strcmp(path, "/") != 0) {
			PANIC("Non-root mount before root File System mounted");
		}
	} else {
		/* Look up the destination directory */
		n = vfs_lookup(path, VFS_DIRECTORY);
		if (!n) {
			DEBUG(DL_DBG, ("vfs_lookup(%s) not found.\n", path));
			goto out;
		}

		/* Check whether it is being used as a mount point already */
		if (n->mount->root == n) {
			rc = -1;
			DEBUG(DL_DBG, ("%s is already a mount point"));
			goto out;
		}
	}

	/* Initialize the mount structure */
	mnt = kmalloc(sizeof(struct vfs_mount), 0);
	if (!mnt) {
		goto out;
	}
	LIST_INIT(&mnt->link);
	mutex_init(&mnt->lock, "fs-mnt-mutex", 0);
	mnt->flags = flags;
	mnt->mnt_point = n;
	mnt->root = NULL;
	mnt->type = NULL;

	/* If a type is specified, look up it */
	if (type) {
		mnt->type = vfs_type_lookup(type);
		if (!mnt->type) {
			rc = -1;
			DEBUG(DL_DBG, ("vfs_type_lookup(%s) not found.\n", type));
			goto out;
		}
	}

	/* Call the File System's mount function to do the mount */
	ASSERT(mnt->type->mount != NULL);
	rc = mnt->type->mount(mnt, 0);
	if (rc != 0) {
		DEBUG(DL_DBG, ("mount failed, err(%d).\n", rc));
		goto out;
	} else if (!mnt->root) {
		PANIC("Mount with root not set");
	}

	/* Append the mount to the mount list */
	list_add_tail(&mnt->link, &_mount_list);
	if (!_root_mount) {
		_root_mount = mnt;
	}

	DEBUG(DL_DBG, ("mounted on %s.\n", path));

 out:
	if (rc != 0) {
		;
	}
	mutex_release(&_mount_list_lock);

	return rc;
}

static int vfs_unmount_internal(struct vfs_node *n)
{
	int rc = -1;

	return rc;
}

/* Unmount a file system */
int vfs_unmount(const char *path)
{
	int rc = -1;
	struct vfs_node *n;

	if (!path) {
		goto out;
	}

	/* Acquire the mount lock first */
	mutex_acquire(&_mount_list_lock);

	n = vfs_lookup(path, VFS_DIRECTORY);
	if (n) {
		rc = vfs_unmount_internal(n);
	} else {
		DEBUG(DL_DBG, ("vfs_lookup(%s) not found.\n", path));
	}

	/* Release the mount lock */
	mutex_release(&_mount_list_lock);
	
 out:
	return rc;
}

/* Initialize the File System layer */
void init_fs()
{
	/* Initialize the fs list lock and mount list lock */
	mutex_init(&_fs_list_lock, "fs-mutex", 0);
	mutex_init(&_mount_list_lock, "mnt-mutex", 0);

	/* Initialize the vfs node cache */
	slab_cache_init(&_vfs_node_cache, "vfs-cache", sizeof(struct vfs_node),
			NULL, NULL, 0);
}
