#ifndef __FS_H__
#define __FS_H__

#include <types.h>
#include "mutex.h"

struct vfs_mount;

/* Structure contains File System type description */
struct vfs_type {
	struct list link;

	const char *name;
	const char *desc;
	int ref_count;

	/* Mount an instance of this File System */
	int (*mount)(struct vfs_mount *mnt, size_t cnt);
};

/* Structure contains detail of a mounted File System */
struct vfs_mount {
	struct list link;
	struct mutex lock;

	int flags;
	
	struct vfs_node *root;
	struct vfs_node *mnt_point;

	struct vfs_type *type;
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
	uint32_t offset;
	uint32_t inode;
	uint32_t impl;
	struct vfs_mount *mount;	// Pointer to the File System mounted on this node
	struct vfs_node *ptr;
	uint32_t (*read)(struct vfs_node *, uint32_t, uint32_t, uint8_t *);
	uint32_t (*write)(struct vfs_node *, uint32_t, uint32_t, uint8_t *);
	void (*open)(struct vfs_node *);
	void (*close)(struct vfs_node *);
	struct dirent *(*readdir)(struct vfs_node *, uint32_t);
	struct vfs_node *(*finddir)(struct vfs_node *, char *);
};

/* Types for vfs_node */
#define VFS_FILE	0x01
#define VFS_DIRECTORY	0x02
#define VFS_CHARDEVICE	0x03
#define VFS_BLOCKDEVICE	0x04
#define VFS_PIPE	0x05
#define VFS_SYMLINK	0x06
#define VFS_MOUNTPOINT	0x08

extern struct vfs_node *_root_node;

extern int vfs_type_register(struct vfs_type *type);
extern int vfs_type_unregister(struct vfs_type *type);
extern int vfs_mount(const char *path, const char *type, const char *opts);
extern int vfs_unmount(const char *path);
extern int vfs_node_refer(struct vfs_node *node);
extern int vfs_node_deref(struct vfs_node *node);
extern struct vfs_node *vfs_node_alloc(uint32_t type);
extern void vfs_node_free(struct vfs_node *node);
extern uint32_t vfs_read(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer);
extern uint32_t vfs_write(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer);
extern void vfs_open(struct vfs_node *node);
extern void vfs_close(struct vfs_node *node);
extern struct dirent *vfs_readdir(struct vfs_node *node, uint32_t index);
extern struct vfs_node *vfs_finddir(struct vfs_node *node, char *name);
extern struct vfs_node *vfs_clone(struct vfs_node *src);
extern struct vfs_node *vfs_lookup(const char *path, int flags);
extern int vfs_create(const char *path, int type, struct vfs_node **node);
extern void init_fs();

#endif	/* __FS_H__ */
