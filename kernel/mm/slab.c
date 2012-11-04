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

/* Slab magazine structure */
struct slab_magazine {
	/* Array of objects in the magazine */
	void *objects[SLAB_MAGAZINE_SIZE];

	struct list link;		// Link to depot lists
};
typedef struct slab_magazine slab_magazine_t;

/* Slab per-CPU cache structure */
struct slab_percpu {
	struct slab_magazine *loaded;	// Current loaded magazine
	struct slab_magazine *previous;	// Previous magazine
};
typedef struct slab_percpu slab_percpu_t;

/* Slab buffer control structure */
struct slab_bufctl {
	struct slab_bufctl *next;	// Address of next buffer
	
	struct slab *parent;		// Parent slab structure
	void *object;			// Pointer to actual object
};
typedef struct slab_bufctl slab_bufctl_t;

/* Slab structure */
struct slab {
	struct list link;		// Link to appropriate slab list in cache
	
	void *base;			// Base address of allocation
	size_t ref_count;		// Reference count
	slab_bufctl_t *free;		// List of free buffers
	size_t color;			// Color of the slab
	slab_cache_t *parent;		// Cache containing the slab
};
typedef struct slab slab_t;

/* Reclaim priority to use for caches */
#define SLAB_DEFAULT_PRIORITY	0
#define SLAB_METADATA_PRIORITY	1
#define SLAB_MAG_PRIORITY	2

/* Internally used caches */
static slab_cache_t _slab_cache_cache;
static slab_cache_t _slab_mag_cache;
static slab_cache_t _slab_bufctl_cache;
static slab_cache_t _slab_slab_cache;
static slab_cache_t *_slab_percpu_cache = NULL;

/* List of all slab caches */
static struct list _slab_caches = {
	.prev = &_slab_caches,
	.next = &_slab_caches
};

static void slab_destroy(slab_cache_t *cache, slab_t *slab)
{
	void *addr;
	slab_bufctl_t *bufctl;

	ASSERT(!slab->ref_count);
	
	addr = slab->base;

	list_del(&slab->link);

	/* Destroy all buffer control structures and the slab structure if
	 * stored externally
	 */
	if (FLAG_ON(cache->flags, SLAB_CACHE_LARGE)) {
		;
	}

	cache->nr_slabs--;
	kmem_free(addr);
}

static slab_t *slab_create(slab_cache_t *cache, int mmflag)
{
	slab_t *slab;
	void *addr;
	size_t i;
	slab_bufctl_t *bufctl, *prev;

	/* Allocate a new slab */
	addr = kmem_alloc(cache->slab_size, mmflag);
	if (!addr) {
		return NULL;
	}

	/* Create the slab structure for the slab */
	if (FLAG_ON(cache->flags, SLAB_CACHE_LARGE)) {
		slab = slab_cache_alloc(&_slab_cache_cache, mmflag);
		if (!slab) {
			kmem_free(addr);
			return NULL;
		}
	} else {
		slab = (struct slab *)((uint32_t)addr + cache->slab_size) -
			sizeof(struct slab);
	}

	cache->nr_slabs++;

	slab->base = addr;
	slab->ref_count = 0;
	LIST_INIT(&slab->link);
	slab->parent = cache;

	/* Divide the buffer into unconstructed, free objects */
	for (i = 0; i < cache->nr_objs; i++) {
		if (FLAG_ON(cache->flags, SLAB_CACHE_LARGE)) {
			bufctl = slab_cache_alloc(&_slab_bufctl_cache, mmflag);
			if (!bufctl) {
				slab_destroy(cache, slab);
				return NULL;
			}

			bufctl->parent = slab;
			bufctl->object = (void *)((uint32_t)addr + slab->color +
						  (i * cache->obj_size));
		} else {
			bufctl = (slab_bufctl_t *)((uint32_t)addr + slab->color +
						   (i * cache->obj_size));
		}

		/* Add to the free list */
		bufctl->next = NULL;
		if (!prev) {
			slab->free = bufctl;
		} else {
			prev->next = bufctl;
		}

		prev = bufctl;
	}

	/* Success - update the cache color and return. Do not add to any
	 * slab lists - the caller will do so.
	 */
	cache->color_next += cache->align;
	if (cache->color_next > cache->color_max) {
		cache->color_next = 0;
	}
	
	return slab;
}

static void *slab_obj_alloc(slab_cache_t *cache, int mmflag)
{
	void *obj;
	slab_bufctl_t *bufctl;
	struct list *l;
	slab_t *slab;

	/* If there is a slab in the partial list, take it */
	if (!LIST_EMPTY(&cache->slab_partial)) {
		l = cache->slab_partial.next;
		slab = LIST_ENTRY(l, slab_t, link);
	} else {
		/* No slabs with free objects available - allocate a new one */
		slab = slab_create(cache, mmflag);
		if (!slab) {
			return NULL;
		}
	}

	ASSERT(slab->free);

	/* Take an object from the slab. If the metadata is stored externally,
	 * then the object address is contained in the object field of the bufctl
	 * structure.
	 */
	bufctl = slab->free;
	slab->free = bufctl->next;
	slab->ref_count++;

	obj = FLAG_ON(cache->flags, SLAB_CACHE_LARGE) ?
		bufctl->object : (void *)bufctl;

	/* Place the allocation on allocation hash table if required */
	if (FLAG_ON(cache->flags, SLAB_CACHE_LARGE)) {
		;
	}

	/* Check if a list move is required */
	if (slab->ref_count == cache->nr_objs) {
		list_add_tail(&slab->link, &cache->slab_full);
	} else {
		list_add_tail(&slab->link, &cache->slab_partial);
	}

	/* Construct the object and return it */
	if (cache->ctor) {
		cache->ctor(obj);
	}

	return obj;
}

static void slab_obj_free(slab_cache_t *cache, void *obj)
{
	slab_t *slab;
	slab_bufctl_t *bufctl;

	/* Find the buffer control structure. */
	if (FLAG_ON(cache->flags, SLAB_CACHE_LARGE)) {
		;
	} else {
		bufctl = (slab_bufctl_t *)obj;
		/* Find the slab corresponding to the object. */
		slab = (slab_t *)((uint32_t)obj + (cache->slab_size - sizeof(slab_t)));
		if (slab->parent != cache) {
			PANIC("Free invalid slab structure");
		}
	}

	/* Call the object destructor */
	if (cache->dtor) {
		cache->dtor(obj);
	}

	ASSERT(slab->ref_count);

	/* Return the object to the slab's free list */
	bufctl->next = slab->free;
	slab->free = bufctl;

	slab->ref_count--;
	if (slab->ref_count == 0) {
		/* Slab empty, destroy it */
		slab_destroy(cache, slab);
	} else if (slab->ref_count + 1 == cache->nr_objs) {
		/* Move from full list to partial list */
		list_add_tail(&slab->link, &cache->slab_partial);
	}
}

static void *slab_cpu_obj_alloc(slab_cache_t *cache)
{
	void *ret;
	boolean_t state;

	state = irq_disable();

	irq_restore(state);

	return ret;
}

static boolean_t slab_cpu_obj_free(slab_cache_t *cache, void *obj)
{
	boolean_t state;

	state = irq_disable();

	irq_restore(state);
	
	return TRUE;
}

void *slab_cache_alloc(slab_cache_t *cache, int mmflag)
{
	void *ret;

	ASSERT(cache != NULL);
	
	if (!FLAG_ON(cache->flags, SLAB_CACHE_NOMAG)) {
		ret = slab_cpu_obj_alloc(cache);
		if (ret) {
			DEBUG(DL_DBG, ("slab: allocated %p from cache %s (magazine).\n",
				       ret, cache->name));
			return ret;
		}
	}

	/* Cannot allocate from magazine layer, allocate from slab layer */
	ret = slab_obj_alloc(cache, mmflag);
	if (ret) {
		DEBUG(DL_DBG, ("slab: allocated %p from cache %s (slab).\n",
			       ret, cache->name));
	}
	
	return ret;
}

void slab_cache_free(slab_cache_t *cache, void *obj)
{
	boolean_t ret;
	
	ASSERT(cache != NULL);

	if (!FLAG_ON(cache->flags, SLAB_CACHE_NOMAG)) {
		ret = slab_cpu_obj_free(cache, obj);
		if (ret) {
			DEBUG(DL_DBG, ("slab: freed %p to cache %s (magazine).\n",
				       obj, cache->name));
			return;
		}
	}

	/* Cannot free to magazine layer, free to slab layer */
	slab_obj_free(cache, obj);

	DEBUG(DL_DBG, ("slab: freed %p to cache %s (slab).\n",
		       obj, cache->name));
}

static int slab_cache_init(slab_cache_t *cache, const char *name, size_t size,
			   slab_ctor_t ctor, slab_dtor_t dtor, int priority,
			   int flags, int mmflag)
{
	slab_cache_t *exist;
	struct list *l;

	ASSERT(size);

	LIST_INIT(&cache->slab_partial);
	LIST_INIT(&cache->slab_full);
	LIST_INIT(&cache->link);

	cache->nr_slabs = 0;
	
	strncpy(cache->name, name, SLAB_NAME_MAX);
	cache->name[SLAB_NAME_MAX - 1] = 0;

	cache->flags = flags;
	cache->ctor = ctor;
	cache->dtor = dtor;
	cache->priority = priority;
	cache->color_next = 0;

	cache->slab_size = PAGE_SIZE;
	cache->nr_objs = cache->slab_size / cache->obj_size;

	/* Add the cache to the global cache list, keeping it ordered by priority  */
	LIST_FOR_EACH(l, &_slab_caches) {
		exist = LIST_ENTRY(l, slab_cache_t, link);
		if (exist->priority > priority) {
			break;
		}
	}
		
	list_add(&cache->link, l->prev);

	DEBUG(DL_DBG, ("slab_cache_init: cache created %s\n", cache->name));
	
	return 0;
}

slab_cache_t *slab_cache_create(const char *name, size_t size, size_t align,
				slab_ctor_t ctor, slab_dtor_t dtor, void *data,
				int flags, int mmflag)
{
	int rc;
	slab_cache_t *cache;

	cache = slab_cache_alloc(&_slab_cache_cache, mmflag);
	if (!cache) {
		return NULL;
	}

	rc = slab_cache_init(cache, name, size, ctor, dtor, SLAB_DEFAULT_PRIORITY, flags, mmflag);
	if (rc != 0) {
		slab_cache_free(&_slab_cache_cache, cache);
		return NULL;
	}
	
	return cache;
}

void slab_cache_destroy(slab_cache_t *cache)
{
	ASSERT(cache);
	
	if (!LIST_EMPTY(&cache->slab_partial) || !LIST_EMPTY(&cache->slab_full)) {
		PANIC("cache still has allocation during destruction");
	}
	
	list_del(&cache->link);

	slab_cache_free(&_slab_cache_cache, cache);
}

/* Initialize the slab allocator */
void init_slab()
{
	/* Initialize the cache for cache structures. */
	slab_cache_init(&_slab_cache_cache, "slab_cache_cache", sizeof(slab_cache_t),
			NULL, NULL, SLAB_METADATA_PRIORITY, 0, MM_BOOT_F);

	/* Initialize the magazine cache. */
	slab_cache_init(&_slab_mag_cache, "slab_cache_cache", sizeof(slab_magazine_t),
			NULL, NULL, SLAB_MAG_PRIORITY, SLAB_CACHE_NOMAG, MM_BOOT_F);

	/* Initialize other internal cache */
	slab_cache_init(&_slab_bufctl_cache, "slab_bufctl_cache", sizeof(slab_bufctl_t),
			NULL, NULL, SLAB_METADATA_PRIORITY, 0, MM_BOOT_F);
	slab_cache_init(&_slab_slab_cache, "slab_slab_cache", sizeof(slab_t),
			NULL, NULL, SLAB_METADATA_PRIORITY, 0, MM_BOOT_F);
}
