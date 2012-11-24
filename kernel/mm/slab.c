#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "hal/hal.h"
#include "mm/mm.h"
#include "mm/kmem.h"
#include "mm/malloc.h"
#include "mm/slab.h"

struct slab;

/* Slab structure */
struct slab {
	struct list link;		// Link to appropriate slab list in cache
	
	slab_cache_t *parent;		// Cache containing the slab
	void *base;			// Base address of allocation
};
typedef struct slab slab_t;

/* List of all slab caches */
static struct list _slab_caches = {
	.prev = &_slab_caches,
	.next = &_slab_caches
};

static slab_t *slab_create(slab_cache_t *cache, int mmflag)
{
	slab_t *slab;

	/* Allocate a new slab */
	slab = (slab_t *)kmem_alloc(cache->obj_size + sizeof(slab_t), mmflag);
	if (slab) {
		LIST_INIT(&slab->link);
		slab->parent = cache;
		slab->base = ((u_char *)slab) + sizeof(slab_t);
		cache->nr_slabs++;
	}

	return slab;
}

static void slab_destroy(slab_cache_t *cache, slab_t *slab)
{
	/* Free an allocated slab */
	list_del(&slab->link);
	cache->nr_slabs--;
	kmem_free(slab);
}

void *slab_cache_alloc(slab_cache_t *cache)
{
	slab_t *slab;
	struct list *l;
	void *ret = NULL;

	ASSERT(cache != NULL);

	if (!LIST_EMPTY(&cache->slab_head)) {
		/* Pop the last entry from the slab list */
		l = cache->slab_head.prev;
		list_del(l);
		slab = LIST_ENTRY(l, slab_t, link);
		ret = slab->base;
	}

	if (ret == NULL) {
		slab = slab_create(cache, 0);
		if (slab) {
			ret = slab->base;
			DEBUG(DL_INF, ("allocated %p from cache %s (slab).\n",
				       ret, cache->name));
		}
	}

	if (ret && cache->ctor) {
		cache->ctor(ret);
	}
	
	return ret;
}

void slab_cache_free(slab_cache_t *cache, void *obj)
{
	slab_t *slab;
	
	ASSERT(cache != NULL && obj != NULL);

	if (cache->dtor) {
		cache->dtor(obj);
	}

	slab = ((u_char *)obj) - sizeof(slab_t);

	list_add_tail(&slab->link, &cache->slab_head);

	DEBUG(DL_DBG, ("freed %p to cache %s (slab).\n",
		       obj, cache->name));
}

void slab_cache_init(slab_cache_t *cache, const char *name, size_t size,
		     slab_ctor_t ctor, slab_dtor_t dtor, int flags)
{
	struct list *l;

	ASSERT(size);

	LIST_INIT(&cache->slab_head);
	LIST_INIT(&cache->link);

	cache->obj_size = size;
	cache->nr_slabs = 0;
	
	strncpy(cache->name, name, SLAB_NAME_MAX);
	cache->name[SLAB_NAME_MAX - 1] = 0;

	cache->flags = flags;
	cache->ctor = ctor;
	cache->dtor = dtor;

	list_add(&cache->link, &_slab_caches);

	DEBUG(DL_DBG, ("cache created %s\n", cache->name));
}

void slab_cache_delete(slab_cache_t *cache)
{
	ASSERT(cache);
	
	if (!LIST_EMPTY(&cache->slab_head)) {
		PANIC("cache still has allocation during destruction");
	}
	
	list_del(&cache->link);
}

/* Initialize the slab allocator */
void init_slab()
{
	;
}
