#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "mm/malloc.h"
#include "debug.h"
#include "rtl/avltree.h"

static int avl_tree_subtree_height(struct avl_tree_node *node)
{
	int left, right;

	if (!node) {
		return 0;
	}

	/* Get the heights of the children and add 1 to account for the node itself*/
	left = (node->left) ? (node->left->height + 1) : 1;
	right = (node->right) ? (node->right->height + 1) : 1;

	/* Store the largest of the heights and return it */
	node->height = (right > left) ? right : left;
	return node->height;
}

static void avl_tree_rotate_left(struct avl_tree *tree, struct avl_tree_node *node)
{
	struct avl_tree_node *child;

	/* Store the node's current right child */
	child = node->right;

	/* Node takes ownership of the child's left child as its right child
	 * (replacing the existing right child).
	 */
	node->right = child->left;
	if (node->right) {
		node->right->parent = node;
	}

	/* Reparent the child to node's parent. */
	child->parent = node->parent;
	if (!child->parent) {
		/* If parent becomes NULL we're at the root of the tree. */
		tree->root = child;
	} else {
		if (child->parent->left == node) {
			child->parent->left = child;
		} else {
			child->parent->right = child;
		}
	}

	/* Child now takes ownership of the old root node as its left child */
	child->left = node;
	child->left->parent = child;
}

static void avl_tree_rotate_right(struct avl_tree *tree, struct avl_tree_node *node)
{
	struct avl_tree_node *child;

	/* Store the node's current left child */
	child = node->left;

	/* Node takes ownership of the child's right child as its left child
	 * (replacing the existing left child).
	 */
	node->left = child->right;
	if (node->left) {
		node->left->parent = node;
	}

	/* Reparent the child to node's parent. */
	child->parent = node->parent;
	if (!child->parent) {
		/* If parent becomes NULL we're at the root of the tree */
		tree->root = child;
	} else {
		if (child->parent->left == node) {
			child->parent->left = child;
		} else {
			child->parent->right = child;
		}
	}

	/* Child now takes ownership of the old root node as its right child */
	child->right = node;
	child->right->parent = child;
}

static int avl_tree_balance_factor(struct avl_tree_node *node)
{
	return avl_tree_subtree_height(node->right) - avl_tree_subtree_height(node->left);
}

static void avl_tree_balance_node(struct avl_tree *tree, struct avl_tree_node *node,
				  int balance)
{
	if (balance > 1) {
		/* Tree is right-heavy, check whether a LR rotation is necessary
		 * (if the right subtree is left-heavy). Note that if the tree is
		 * right-heavy, then node->right is guaranteed not to be a null
		 * pointer.
		 */
		if (avl_tree_balance_factor(node->right) < 0) {
			/* LR rotation. Perform a right rotation of the right subtree */
			avl_tree_rotate_right(tree, node->right);
		}

		avl_tree_rotate_left(tree, node);
	} else if (balance < -1) {
		/* Tree is left-heavy, check whether a RL rotation is necessary
		 * (if the left subtree is right-heavy).
		 */
		if (avl_tree_balance_factor(node->left) > 0) {
			/* RL rotation. Perform a left rotation of the left subtree */
			avl_tree_rotate_left(tree, node->left);
		}

		avl_tree_rotate_right(tree, node);
	}
}

static struct avl_tree_node *avl_tree_lookup_internal(struct avl_tree *tree, key_t key)
{
	struct avl_tree_node *node = tree->root;

	/* Descend down the tree to find the required node */
	while (node) {
		if (node->key > key) {
			node = node->left;
		} else if (node->key < key) {
			node = node->right;
		} else {
			return node;
		}
	}
	
	return NULL;
}

void avl_tree_insert_node(struct avl_tree *tree, struct avl_tree_node *node,
			  key_t key, void *value)
{
	struct avl_tree_node **next, *curr = NULL;
	int balance;

	node->left = NULL;
	node->right = NULL;
	node->height = 0;
	node->key = key;
	node->value = value;

	/* If tree is currently empty, just insert and finish */
	if (!tree->root) {
		node->parent = NULL;
		tree->root = node;
		return;
	}

	/* Determine where we need to insert the node */
	next = &tree->root;
	while (*next) {
		curr = *next;

		/* Ensure that the key is unique */
		if (key == curr->key) {
			PANIC("Attempt to insert duplicated key into AVL tree.");
		}

		/* Get the next pointer */
		next = (key > curr->key) ? &curr->right : &curr->left;
	}

	/* We now have an insertion point for the new node */
	node->parent = curr;
	*next = node;

	/* Now go back up the tree and check its balance */
	while (curr) {
		balance = avl_tree_balance_factor(curr);
		if (balance < -1 || balance > 1) {
			avl_tree_balance_node(tree, curr, balance);
		}
		curr = curr->parent;
	}
}

void avl_tree_remove_node(struct avl_tree *tree, struct avl_tree_node *node)
{
	struct avl_tree_node *child, *start;
	int balance;

	/* First we need to detach the node from the tree */
	if (node->left) {
		/* Left node exists. Descend onto it, and then find the right-most
		 * node, which will replace the node that we're removing.
		 */
		child = node->left;
		while (child->right) {
			child = child->right;
		}

		if (child != node->left) {
			if (child->left) {
				/* There is a left subtree. This must be moved up
				 * to replace child
				 */
				child->left->parent = child->parent;
				child->parent->right = child->left;
				start = child->left;
			} else {
				/* Detach the child */
				child->parent->right = NULL;
				start = child->parent;
			}

			child->left = node->left;
		} else {
			/* The left child has no right child. It will replace the
			 * node being deleted as-is.
			 */
			start = child;
		}

		/* Replace the node and fix up the pointers */
		child->right = node->right;
		child->parent = node->parent;
		if (child->right) {
			child->right->parent = child;
		}

		if (child->left) {
			child->left->parent = child;
		}

		if (node->parent) {
			if (node->parent->left == node) {
				node->parent->left = child;
			} else {
				node->parent->right = child;
			}
		} else {
			ASSERT(node == tree->root);
			tree->root = child;
		}
	} else if (node->right) {
		/* Left node doesn't exist but right node does. This is easy.
		 * Just replace the node with its right child.
		 */
		node->right->parent = node->parent;
		if (node->parent) {
			if (node->parent->left == node) {
				node->parent->left = node->right;
			} else {
				node->parent->right = node->right;
			}
		} else {
			ASSERT(node == tree->root);
			tree->root = node->right;
		}

		start = node->right;
	} else {
		/* Node is a leaf. If it is the only element in the tree, then
		 * just remove it and return - no rebalancing required. Otherwise,
		 * remove it and then rebalance.
		 */
		if (node->parent) {
			if (node->parent->left == node) {
				node->parent->left = NULL;
			} else {
				node->parent->right = NULL;
			}
		} else {
			ASSERT(node == tree->root);
			tree->root = NULL;
			return;
		}

		start = node->parent;
	}

	/* start now points to where we need to start rebalancing from */
	while (start) {
		balance = avl_tree_balance_factor(start);
		if (balance < -1 || balance > 1) {
			avl_tree_balance_node(tree, start, balance);
		}
		start = start->parent;
	}
}

void avl_tree_insert(struct avl_tree *tree, key_t key, void *value)
{
	struct avl_tree_node *node;

	node = kmalloc(sizeof(struct avl_tree_node), 0);
	ASSERT(node != NULL);
	avl_tree_insert_node(tree, node, key, value);
}

void avl_tree_remove(struct avl_tree *tree, key_t key)
{
	struct avl_tree_node *node;

	node = avl_tree_lookup_internal(tree, key);
	if (node) {
		avl_tree_remove_node(tree, node);
		kfree(node);
	}
}

void *avl_tree_lookup(struct avl_tree *tree, key_t key)
{
	struct avl_tree_node *node;

	node = avl_tree_lookup_internal(tree, key);
	return (node) ? node->value : NULL;
}
