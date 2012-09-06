#ifndef __FD_H__
#define __FD_H__

struct process;

/* Definition of the file descriptor table structure */
struct fd_table {
	size_t len;		// Length of this table
	size_t slots_count;	// Count of the slots in this table
	int ref_count;		// Reference count of this table
	struct vfs_node **nodes;// Pointer to the VFS nodes
};
typedef struct fd_table fd_table_t;

int fd_attach(struct process *p, struct vfs_node *n);
fd_table_t *fd_table_create(fd_table_t *parent);
void fd_table_destroy(fd_table_t *table);
fd_table_t *fd_table_clone(fd_table_t *src);

#endif	/* __FD_H__ */
