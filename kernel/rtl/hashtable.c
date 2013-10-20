#include <types.h>
#include <stddef.h>
#include <string.h>
#include <limit.h>
#include "debug.h"
#include "list.h"
#include "mm/malloc.h"
#include "rtl/hashtable.h"

#define LINK_2_ENTRY(_ht_, _link_)	((void *)(((char *)_link_) - ((_ht_)->link_off)))
#define ENTRY_2_LINK(_ht_, _entry_)	((struct list *)(((char *)_entry_) + ((_ht_)->link_off)))

static int hashtable_lookup_bucket(struct hashtable *ht, void *key,
				   void **val, uint32_t index)
{
	int rc = 0;
	struct list *link = NULL;
	void *value = NULL;
	boolean_t found = FALSE;

	if (index >= ht->nr_buckets) {
		rc = -1;
		goto out;
	}

	LIST_FOR_EACH(link, &ht->buckets[index]) {
		value = LINK_2_ENTRY(ht, link);
		if ((*ht->compare_func)(key, value) == 0) {
			found = TRUE;
			break;
		}
	}

	if (found == FALSE) {
		rc = -1;
		goto out;
	}

 out:
	return rc;
}

static int hashtable_insert_bucket(struct hashtable *ht, void *key,
				   void *val, uint32_t index)
{
	int rc = 0;
	struct list *link = NULL;
	void *value = NULL;

	rc = hashtable_lookup_bucket(ht, key, &value, index);
	if (rc == 0) {
		rc = -1;
		goto out;
	} else {
		rc = 0;
	}

	link = ENTRY_2_LINK(ht, val);
	list_add_tail(link, &ht->buckets[index]);
	ht->nr_entries++;

 out:
	return rc;
}

int hashtable_lookup(struct hashtable *ht, void *key, void **val)
{
	int rc = 0;
	uint32_t hash_val = 0;

	hash_val = (*ht->hash_func)(key, ht->nr_buckets);

	rc = hashtable_lookup_bucket(ht, key, val, hash_val);

	return rc;
}

int hashtable_insert(struct hashtable *ht, void *key, void *val)
{
	int rc = 0;
	uint32_t hash_val = ULONG_MAX;

	hash_val = (*ht->hash_func)(key, ht->nr_buckets);
	ASSERT(hash_val < ht->nr_buckets);

	rc = hashtable_insert_bucket(ht, key, val, hash_val);

	return rc;
}

int hashtable_remove(struct hashtable *ht, void *key)
{
	int rc = 0;
	void *val = NULL;
	struct list *link = NULL;

	rc = hashtable_lookup(ht, key, &val);
	if (rc != 0) {
		rc = -1;
		goto out;
	}

	link = ENTRY_2_LINK(ht, val);
	list_del(link);
	ht->nr_entries--;

 out:
	return rc;
}

void hashtable_init(struct hashtable *ht, struct list *buckets, uint32_t nr_buckets,
		    uint32_t link_off, hashtable_hash_func hash_func,
		    hashtable_compare_func compare_func, int flags)
{
	uint32_t i;
	struct list *bptr;

	ht->buckets = buckets;
	ht->nr_buckets = nr_buckets;
	ht->nr_entries = 0;
	ht->link_off = link_off;
	ht->hash_func = hash_func;
	ht->compare_func = compare_func;
	ht->flags = flags;

	bptr = buckets;
	for (i = 0; i < ht->nr_buckets; i++) {
		LIST_INIT(&bptr[i]);
	}
}

uint32_t hashtable_get_entry_count(struct hashtable *ht)
{
	return ht->nr_entries;
}
