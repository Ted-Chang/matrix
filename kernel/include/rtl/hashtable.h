#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

typedef uint32_t (*hashtable_hash_func)(void *key, uint32_t nr_buckets);
typedef int (*hashtable_compare_func)(void *key, void *entry);

struct hashtable {
	int flags;
	struct list *buckets;
	uint32_t nr_buckets;
	uint32_t nr_entries;
	uint32_t link_off;
	hashtable_hash_func hash_func;
	hashtable_compare_func compare_func;
};

extern int hashtable_lookup(struct hashtable *ht, void *key, void **val);
extern int hashtable_insert(struct hashtable *ht, void *key, void *val);
extern int hashtable_remove(struct hashtable *ht, void *key);
extern uint32_t hashtable_get_entry_count(struct hashtable *ht);
extern void hashtable_init(struct hashtable *ht, struct list *buckets, uint32_t nr_buckets,
			   uint32_t link_off, hashtable_hash_func hash_func,
			   hashtable_compare_func compare_func, int flags);

#endif	/* __HASHTABLE_H__ */
