#ifndef __LIST_H__
#define __LIST_H__

#include <types.h>

/* Doubly linked list node structure */
struct list {
	struct list *prev;
	struct list *next;
};
typedef struct list list_t;


#define list_entry(entry, type, member) \
	(type *)((char *)entry - offsetof(type, member))

#define list_empty(list) \
	(((list)->prev == (list)) && ((list)->next == (list)))

#endif	/* __LIST_H__ */
