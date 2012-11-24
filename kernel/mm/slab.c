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
	uint32_t magic;
	struct list link;		// Link to appropriate slab list in cache
	slab_cache_t *parent;		// Cache containing the slab
	void *base;			// Base address of allocation
};
typedef struct slab slab_t;

#define SLAB_MAGIC	'BALS'

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
		slab->magic = SLAB_MAGIC;
		LIST_INIT(&slab->link);
		slab->parent = cache;
		slab->base = ((u_char *)slab) + sizeof(slab_t);
		cache->nr_slabs++;
	}

	return slab;
}

static void slab_destroy(slab_cache_t *cache, slab_t *slab)
{
	ASSERT(slab->magic == SLAB_MAGIC);
	/* Free an allocated slab */
	list_del(&slab->link);
	cache->nr_slabs--;
	kmem_free(slab);
}

void *slab_cache_alloc(slab_cache_t *cache)
{
	slab_t *slab;
	struct list *l;
	void *obj = NULL;

	ASSERT(cache != NULL);

	if (!LIST_EMPTY(&cache->slab_head)) {
		/* Pop the last entry from the slab list */
		l = cache->slab_head.prev;
		list_del(l);
		slab = LIST_ENTRY(l, slab_t, link);
		ASSERT(slab->magic == SLAB_MAGIC);
		obj = slab->base;
	}

	if (obj == NULL) {
		slab = slab_create(cache, 0);
		if (slab) {
			obj = slab->base;
			DEBUG(DL_INF, ("allocated %p from cache %s.\n",
				       obj, cache->name));
		}
	}

	if (obj && cache->ctor) {
		cache->ctor(obj);
	}
	
	return obj;
}

void slab_cache_free(slab_cache_t *cache, void *obj)
{
	slab_t *slab;
	
	ASSERT(cache != NULL && obj != NULL);

	if (cache->dtor) {
		cache->dtor(obj);
	}

	slab = (slab_t *)(((u_char *)obj) - sizeof(slab_t));
	ASSERT(slab->magic == SLAB_MAGIC);

	if (cache->nr_slabs < cache->color_max) {
		ASSERT(LIST_EMPTY(&slab->link));
		list_add_tail(&slab->link, &cache->slab_head);
	} else {
		slab_destroy(cache, slab);
		DEBUG(DL_INF, ("freed %p to cache %s.\n",
			       obj, cache->name));
	}
}

void slab_cache_init(slab_cache_t *cache, const char *name, size_t size,
		     slab_ctor_t ctor, slab_dtor_t dtor, int flags)
{
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

	cache->color_next = 0;
	cache->color_max = 256;

	list_add(&cache->link, &_slab_caches);

	DEBUG(DL_DBG, ("cache created %s\n", cache->name));
}

void slab_cache_delete(slab_cache_t *cache)
{
	slab_t *slab;
	struct list *l;
	
	ASSERT(cache);
	
	while (!LIST_EMPTY(&cache->slab_head)) {
		l = cache->slab_head.next;
		list_del(l);
		slab = LIST_ENTRY(l, slab_t, link);
		ASSERT(slab->parent == cache);
		slab_destroy(cache, slab->base);
	}
	
	list_del(&cache->link);
}

/* Initialize the slab allocator */
void init_slab()
{
	;
}
