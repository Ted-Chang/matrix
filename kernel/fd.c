#include <types.h>
#include <stddef.h>
#include <string.h>
#include "fd.h"
#include "fs.h"
#include "proc/process.h"

int fd_attach(struct process *p, struct vfs_node *n)
{
	struct vfs_node **new_slots;
	size_t slots_count;
	
	if (p->fds->len == p->fds->slots_count) {
		slots_count = p->fds->slots_count * 2;
		new_slots = (struct vfs_node **)
			kmem_alloc(slots_count * sizeof(struct vfs_node *));
		if (!new_slots)
			return -1;
		
		/* Copy the original nodes to the new slots */
		memcpy(new_slots, p->fds->nodes, p->fds->len * sizeof(struct vfs_node *));
		kmem_free(p->fds->nodes);		// Free the original nodes
		p->fds->nodes = new_slots;
		p->fds->slots_count = slots_count;
	}

	p->fds->nodes[p->fds->len] = n;
	p->fds->len++;

	return (int)p->fds->len - 1;
}

fd_table_t *fd_table_create(fd_table_t *parent)
{
	fd_table_t *t;
	size_t nodes_len, i;

	t = (fd_table_t *)kmem_alloc(sizeof(fd_table_t));
	if (!t)
		goto out;
	
	t->ref_count = 1;
	if (!parent) {
		t->len = 3;
		t->slots_count = 4;
	} else {
		t->len = parent->len;
		t->slots_count = parent->slots_count;
	}
	
	nodes_len = t->slots_count * sizeof(struct vfs_node *);
	t->nodes = (struct vfs_node **)kmem_alloc(nodes_len);
	if (!t->nodes) {
		kmem_free(t);
		t = NULL;
		goto out;
	}
	memset(t->nodes, 0, nodes_len);
	
	/* Inherit all inheritable file descriptors in the parent table */
	if (parent) {
		for (i = 0; i < t->len; i++) {
			t->nodes[i] = vfs_clone(parent->nodes[i]);
		}
	}

out:
	return t;
}

void fd_table_destroy(fd_table_t *table)
{
	size_t i;
	
	for (i = 0; i < table->len; i++) {
		//vfs_close(t->nodes[i]);
	}
	
	if (table->nodes)
		kmem_free(table->nodes);
	
	kmem_free(table);
}

fd_table_t *fd_table_clone(fd_table_t *src)
{
	;
}
