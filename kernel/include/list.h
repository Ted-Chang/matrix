#ifndef __LIST_H__
#define __LIST_H__

#include <types.h>

/* Doubly linked list node structure */
struct list {
	struct list *prev;
	struct list *next;
};
typedef struct list list_t;

#define LIST_INIT(list) \
	(((list)->prev) = ((list)->next) = (list))

#define LIST_ENTRY(entry, type, member) \
	(type *)((char *)entry - offsetof(type, member))

#define LIST_EMPTY(list) \
	(((list)->prev == (list)) && ((list)->next == (list)))

#define LIST_FOR_EACH(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define LIST_FOR_EACH_SAFE(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != head; pos = n, n = pos->next)

static INLINE void __list_add(struct list *new, struct list *prev,
			      struct list *next)
{
	next->prev = new;
	prev->next = new;
	new->next = next;
	new->prev = prev;
}

/**
 * Insert a new entry after the specified head
 */
static INLINE void list_add(struct list *new, struct list *head)
{
	__list_add(new, head, head->next);
}

/**
 * Insert a new entry before the specified head
 */
static INLINE void list_add_tail(struct list *new, struct list *head)
{
	__list_add(new, head->prev, head);
}

static INLINE void __list_del(struct list *prev, struct list *next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * Delete the specified entry from the list
 */
static INLINE void list_del(struct list *entry)
{
	__list_del(entry->prev, entry->next);
	entry->prev = entry;
	entry->next = entry;
}

/**
 * Move the specified entry from the list to another
 */
static INLINE void list_move(struct list *link, struct list *head)
{
	__list_del(link->prev, link->next);
	list_add(link, head);
}

#endif	/* __LIST_H__ */
