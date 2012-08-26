#ifndef __FS_H__
#define __FS_H__

#include <types.h>

struct dirent {
	char name[128];	// File name
	uint32_t ino;	// Inode number
};

struct vfs_node {
	char name[128];
	uint32_t flags;
	uint32_t mask;
	uint32_t uid;
	uint32_t gid;
	uint32_t length;
	uint32_t inode;
	uint32_t impl;
	struct vfs_node *ptr;
	uint32_t (*read)(struct vfs_node *, uint32_t, uint32_t, uint8_t *);
	uint32_t (*write)(struct vfs_node *, uint32_t, uint32_t, uint8_t *);
	void (*open)(struct vfs_node *);
	void (*close)(struct vfs_node *);
	struct dirent *(*readdir)(struct vfs_node *, uint32_t);
	struct vfs_node *(*finddir)(struct vfs_node *, char *);
};

/* Flags for vfs_node */
#define VFS_FILE	0x01
#define VFS_DIRECTORY	0x02
#define VFS_CHARDEVICE	0x03
#define VFS_BLOCKDEVICE	0x04
#define VFS_PIPE	0x05
#define VFS_SYMLINK	0x06
#define VFS_MOUNTPOINT	0x08

extern struct vfs_node *root_node;

uint32_t vfs_read(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t vfs_write(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer);
void vfs_open(struct vfs_node *node);
void vfs_close(struct vfs_node *node);
struct dirent *vfs_readdir(struct vfs_node *node, uint32_t index);
struct vfs_node *vfs_finddir(struct vfs_node *node, char *name);
struct vfs_node *vfs_clone(struct vfs_node *src);

#endif	/* __FS_H__ */
