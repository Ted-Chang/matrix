#include <types.h>
#include <stddef.h>
#include "debug.h"
#include "proc/process.h"
#include "fs.h"
#include "ioctx.h"

extern struct vfs_mount *_root_mount;

void io_init_ctx(struct io_ctx *ctx, struct io_ctx *parent)
{
	ctx->cwd = NULL;
	ctx->rd = NULL;

	if (parent) {
		ASSERT(parent->rd != NULL);
		ASSERT(parent->cwd != NULL);
		
		vfs_node_refer(parent->rd);
		ctx->rd = parent->rd;
		
		vfs_node_refer(parent->cwd);
		ctx->cwd = parent->cwd;
	} else if (_root_mount) {
		vfs_node_refer(_root_mount->root);
		ctx->rd = _root_mount->root;
		
		vfs_node_refer(_root_mount->root);
		ctx->cwd = _root_mount->root;
	} else {
		/* This should be the only case when kernel process is
		 * being created
		 */
		ASSERT(!_kernel_proc);
	}
}

void io_destroy_ctx(struct io_ctx *ctx)
{
	ASSERT((ctx->cwd != NULL) && (ctx->rd != NULL));
	vfs_node_deref(ctx->cwd);
	vfs_node_deref(ctx->rd);
}

int io_setcwd(struct io_ctx *ctx, struct vfs_node *node)
{
	int rc = -1;
	
	if (node->type != VFS_DIRECTORY) {
		goto out;
	}

	vfs_node_refer(node);
	ASSERT(ctx->cwd != NULL);
	vfs_node_deref(ctx->cwd);
	ctx->cwd = node;
	rc = 0;

 out:
	return rc;
}

int io_setroot(struct io_ctx *ctx, struct vfs_node *node)
{
	int rc = -1;

	if (node->type != VFS_DIRECTORY) {
		goto out;
	}

	/* Reference the node twice */
	vfs_node_refer(node);
	vfs_node_refer(node);

	ASSERT((ctx->cwd != NULL) && (ctx->rd != NULL));
	vfs_node_deref(ctx->cwd);
	ctx->cwd = node;
	vfs_node_deref(ctx->rd);
	ctx->rd = node;
	rc = 0;

 out:
	return rc;
}
