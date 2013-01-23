#ifndef __IOCTX_H__
#define __IOCTX_H__

struct vfs_node;

struct io_ctx {
	struct vfs_node *rd;	// Root directory
	struct vfs_node *cwd;	// Current working directory
};

extern void io_init_ctx(struct io_ctx *ctx, struct io_ctx *parent);
extern void io_destroy_ctx(struct io_ctx *ctx);

#endif	/* __IOCTX_H__ */
