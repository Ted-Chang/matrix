#ifndef __FS_H__
#define __FS_H__

#include <types.h>
#include "mutex.h"
#include "dirent.h"
#include "rtl/avltree.h"

struct vfs_mount;
struct vfs_node;

/* Structure contains File System type description */
struct vfs_type {
	struct list link;

	const char *name;
	const char *desc;
	int ref_count;

	/* Mount an instance of this File System */
	int (*mount)(struct vfs_mount *mnt, int flags, const void *data);
};

struct vfs_mount_ops {
	int (*umount)(struct vfs_mount *mnt);
	int (*flush)(struct vfs_mount *mnt);
	int (*read_node)(struct vfs_mount *mnt, ino_t ino, struct vfs_node **np);
};

/* Structure contains detail of a mounted File System */
struct vfs_mount {
	struct list link;
	struct mutex lock;

	struct avl_tree nodes;		// Tree mapping node IDs to node structure

	int flags;
	struct vfs_mount_ops *ops;	// Mount operations
	
	struct vfs_node *root;		// Root node of the mount
	struct vfs_node *mnt_point;	// Directory that the mount is mounted

	struct vfs_type *type;		// File system type
	void *data;			// Pointer to private data
};

/* Structure contains operations can be done on a File System node */
struct vfs_node_ops {
	int (*create)(struct vfs_node *, const char *, uint32_t, struct vfs_node **);
	int (*read)(struct vfs_node *, uint32_t, uint32_t, uint8_t *);
	int (*write)(struct vfs_node *, uint32_t, uint32_t, uint8_t *);
	int (*finddir)(struct vfs_node *, const char *, ino_t *id);
	int (*readdir)(struct vfs_node *, uint32_t, struct dirent **);
	int (*close)(struct vfs_node *);
};

/* Structure contains detail of a File System node */
struct vfs_node {
	char name[128];
	int ref_count;
	uint32_t type;
	uint32_t mask;
	uint32_t uid;
	uint32_t gid;
	uint32_t length;
	uint32_t offset;		// FixMe: offset should be fd specific
	uint32_t ino;			// inode number

	struct vfs_node_ops *ops;	// Node operations
	void *data;			// Pointer to private data
	
	struct vfs_mount *mounted;	// Pointer to the File System mounted on this node
	struct vfs_mount *mount;	// Mount that the node resides on
};

/* Types for vfs_node */
#define VFS_FILE	0x01
#define VFS_DIRECTORY	0x02
#define VFS_CHARDEVICE	0x03
#define VFS_BLOCKDEVICE	0x04
#define VFS_PIPE	0x05
#define VFS_SYMLINK	0x06
#define VFS_MOUNTPOINT	0x08

extern int vfs_type_register(struct vfs_type *type);
extern int vfs_type_unregister(struct vfs_type *type);
extern int vfs_mount(const char *dev, const char *path, const char *type,
		     const void *data);
extern int vfs_umount(const char *path);
extern struct vfs_node *vfs_node_alloc(struct vfs_mount *mnt, uint32_t type,
				       struct vfs_node_ops *ops, void *data);
extern void vfs_node_free(struct vfs_node *node);
extern int vfs_node_refer(struct vfs_node *node);
extern int vfs_node_deref(struct vfs_node *node);
extern int vfs_read(struct vfs_node *node, uint32_t offset, uint32_t size,
		    uint8_t *buffer);
extern int vfs_write(struct vfs_node *node, uint32_t offset, uint32_t size,
		     uint8_t *buffer);
extern int vfs_finddir(struct vfs_node *node, const char *name, ino_t *id);
extern int vfs_create(const char *path, uint32_t type, struct vfs_node **node);
extern int vfs_close(struct vfs_node *node);
extern int vfs_readdir(struct vfs_node *node, uint32_t index, struct dirent **dentry);
extern struct vfs_node *vfs_clone(struct vfs_node *src);
extern struct vfs_node *vfs_lookup(const char *path, int flags);
extern void init_fs();

#endif	/* __FS_H__ */
