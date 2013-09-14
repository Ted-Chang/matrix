#ifndef __RADIXTREE_H__
#define __RADIXTREE_H__

struct radix_tree_node_ptr {
	struct radix_tree_node *nodes[16];	// Array of nodes
	uint32_t nr_nodes;			// Number of nodes
};

struct radix_tree_node {
	struct radix_tree_node *parent;		// Pointer to parent node

	/* Two level array of child nodes (each level 16 entries) */
	struct radix_tree_node_ptr *children[16];
};

struct radix_tree {
	struct radix_tree_node root;
};

extern void *radix_tree_lookup(struct radix_tree *tree, const char *key);
extern void radix_tree_init(struct radix_tree *tree);
extern void radix_tree_uninit();

#endif	/* __RADIXTREE_H__ */
