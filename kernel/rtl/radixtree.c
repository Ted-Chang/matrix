#include <types.h>
#include <stddef.h>
#include "debug.h"
#include "rtl/radixtree.h"

void radix_tree_init(struct radix_tree *tree)
{
	/* Clear the root node */
	memset(&tree->root, 0, sizeof(tree->root));
}

void radix_tree_uninit(struct radix_tree *tree)
{
	;
}
