#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/matrix.h"
#include "debug.h"
#include "mm/malloc.h"
#include "avltree.h"

void avl_tree_test()
{
	uint32_t i;
	struct avl_tree t;
	struct avl_tree_node n[2];

	uint32_t k[] = {
		1,
		2,
		3,
		4,
	};
	
	char *v[] = {
		"Node1",
		"Node2",
		"Node3",
		"Node4",
	};

	char *str = NULL;

	memset(n, 0, sizeof(struct avl_tree_node) * 2);

	avl_tree_init(&t);

	avl_tree_insert_node(&t, &n[0], k[0], v[0]);
	str = avl_tree_lookup(&t, k[0]);
	if (!str) {
		kprintf("insert key(%d) failed.\n", k[0]);
	} else {
		kprintf("value of key(%d): %s.\n", k[0], str);
	}
	
	avl_tree_insert_node(&t, &n[1], k[1], v[1]);
	str = avl_tree_lookup(&t, k[1]);
	if (!str) {
		kprintf("insert key(%d) failed.\n", k[1]);
	} else {
		kprintf("value of key(%d): %s.\n", k[1], str);
	}
	
	avl_tree_insert(&t, k[2], v[2]);
	str = avl_tree_lookup(&t, k[2]);
	if (!str) {
		kprintf("insert key(%d) failed.\n", k[2]);
	} else {
		kprintf("value of key(%d): %s.\n", k[2], str);
	}
	
	avl_tree_insert(&t, k[3], v[3]);
	str = avl_tree_lookup(&t, k[3]);
	if (!str) {
		kprintf("insert key(%d) failed.\n", k[3]);
	} else {
		kprintf("value of key(%d): %s.\n", k[3], str);
	}

	for (i = 0; i < (sizeof(k)/sizeof(k[0])); i++) {

		str = avl_tree_lookup(&t, k[i]);
		if (!str) {
			kprintf("key(%d) not found.\n", k[i]);
		} else {
			kprintf("key(%d) value(%s).\n", k[i], str);
		}
	}

	for (i = 0; i < (sizeof(k)/sizeof(k[0])); i++) {

		if (i < 2) {
			avl_tree_remove_node(&t, &n[i]);
		} else {
			avl_tree_remove(&t, k[i]);
		}
		
		str = avl_tree_lookup(&t, k[i]);
		if (str) {
			kprintf("key(%d) not removed.\n", k[i]);
		} else {
			kprintf("key(%d) removed.\n", k[i]);
		}
	}
}
