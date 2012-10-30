#include <types.h>
#include <stddef.h>
#include <string.h>
#include "fd.h"
#include "fs.h"
#include "proc/process.h"
#include "matrix/debug.h"
#include "mm/malloc.h"

struct vfs_node *fd_2_vfs_node(struct process *p, int fd)
{
	if (!p)
		p = CURR_PROC;
	
	if (fd < 0 || fd > p->fds->slots_count)
		return NULL;

	return p->fds->nodes[fd];
}

int fd_attach(struct process *p, struct vfs_node *n)
{
	size_t i;
	
	for (i = 0; i < p->fds->slots_count; i++) {
		if (p->fds->nodes[i] == NULL) {
			p->fds->nodes[i] = n;
			break;
		}
	}

	return (int)((i == p->fds->slots_count) ? -1 : i);
}

int fd_detach(struct process *p, int fd)
{
	int rc = 0;

	if (!p)
		p = CURR_PROC;
	
	if (fd >= p->fds->slots_count) {
		rc = -1;
		goto out;
	}
	
	if (!p->fds->nodes[fd]) {
		rc = -1;
		goto out;
	}

	p->fds->nodes[fd] = NULL;

out:
	return rc;
}

fd_table_t *fd_table_create(fd_table_t *parent)
{
	fd_table_t *t;
	size_t nodes_len, i;

	t = (fd_table_t *)kmalloc(sizeof(fd_table_t), 0);
	if (!t)
		goto out;
	
	t->ref_count = 1;
	if (!parent) {
		t->slots_count = 10;
	} else {
		t->slots_count = parent->slots_count;
	}
	
	nodes_len = t->slots_count * sizeof(struct vfs_node *);
	t->nodes = (struct vfs_node **)kmalloc(nodes_len, 0);
	if (!t->nodes) {
		kfree(t);
		t = NULL;
		goto out;
	}
	memset(t->nodes, 0, nodes_len);
	
	/* Inherit all inheritable file descriptors in the parent table */
	if (parent) {
		for (i = 0; i < t->slots_count; i++) {
			if (!parent->nodes[i])
				continue;
			
			t->nodes[i] = vfs_clone(parent->nodes[i]);
		}
	}

	DEBUG(DL_DBG, ("fd_table_create: parent(%x)\n", parent));

out:
	return t;
}

void fd_table_destroy(fd_table_t *table)
{
	size_t i;

	DEBUG(DL_DBG, ("fd_table_destroy: table(%x)\n", table));
	
	for (i = 0; i < table->slots_count; i++) {
		if (table->nodes[i]) {
			vfs_close(table->nodes[i]);
		}
	}
	
	if (table->nodes) {
		kfree(table->nodes);
	}
	
	kfree(table);
}

fd_table_t *fd_table_clone(fd_table_t *src)
{
	return NULL;
}
