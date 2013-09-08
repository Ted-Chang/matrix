#ifndef __AVLTREE_H__
#define __AVLTREE_H__

#include <types.h>

/* Key type definition */
typedef uint64_t key_t;

/* AVL tree node structure */
struct avl_tree_node {
	struct avl_tree_node *parent;	// Parent node
	struct avl_tree_node *left;	// Left-hand child node
	struct avl_tree_node *right;	// Right-hand child node
	int height;			// Height of the node
	key_t key;			// Key of the node
	void *value;			// Value of the node
};
typedef struct avl_tree_node avl_tree_node_t;

/* AVL tree structure */
struct avl_tree {
	struct avl_tree_node *root;
};
typedef struct avl_tree avl_tree_t;

/* Get the entry from the node */
#define AVL_TREE_ENTRY(node, type) \
	((node) ? (type *)(node)->value : NULL)

/* Checks whether the given AVL tree is empty */
#define AVL_TREE_EMPTY(_t_) \
	((_t_)->root == NULL)

#define AVL_TREE_FOR_EACH(pos, _t_) \
	for (pos = avl_tree_first(_t_); pos; \
	     pos = avl_tree_node_next(pos))

#define AVL_TREE_FOR_EACH_SAFE(pos, n, _t_) \
	for (pos = avl_tree_first(_t_), n = avl_tree_node_next(pos); \
	     pos; pos = n, n = avl_tree_node_next(pos))

/* Initialize an AVL tree */
static INLINE void avl_tree_init(struct avl_tree *tree)
{
	tree->root = NULL;
}

extern void avl_tree_insert(struct avl_tree *tree, key_t key, void *value);
extern void avl_tree_remove(struct avl_tree *tree, key_t key);
extern void avl_tree_insert_node(struct avl_tree *tree, struct avl_tree_node *node,
				 key_t key, void *value);
extern void avl_tree_remove_node(struct avl_tree *tree, struct avl_tree_node *node);
extern void *avl_tree_lookup(struct avl_tree *tree, key_t key);
extern struct avl_tree_node *avl_tree_first(struct avl_tree *tree);
extern struct avl_tree_node *avl_tree_last(struct avl_tree *tree);
extern struct avl_tree_node *avl_tree_node_next(struct avl_tree_node *node);

#endif	/* __AVLTREE_H__ */
