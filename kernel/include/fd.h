#ifndef __FD_H__
#define __FD_H__

struct process;

// FixMe: replace the array with list or other data structure
/* Definition of the file descriptor table structure */
struct fd_table {
	size_t slots_count;		// Count of the slots in this table
	int ref_count;			// Reference count of this table
	struct vfs_node **nodes;	// Pointer to the VFS nodes
};
typedef struct fd_table fd_table_t;

extern struct vfs_node *fd_2_vfs_node(struct process *p, int fd);
extern int fd_attach(struct process *p, struct vfs_node *n);
extern int fd_detach(struct process *p, int fd);
extern fd_table_t *fd_table_create(fd_table_t *parent);
extern void fd_table_destroy(fd_table_t *table);
extern fd_table_t *fd_table_clone(fd_table_t *src);
extern struct process *pid_2_process(pid_t pid);

#endif	/* __FD_H__ */
