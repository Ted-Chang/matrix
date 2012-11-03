#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/matrix.h"
#include "matrix/debug.h"
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

static void slab_destroy(slab_cache_t *cache, slab_t *slab)
{
	void *addr;

	addr = slab->base;

	list_del(&slab->link);

	cache->nr_slabs--;
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

void *slab_cache_alloc(slab_cache_t *cache, int mmflag)
{
	void *ret;

	ASSERT(cache != NULL);
	if (!FLAG_ON(cache->flags, SLAB_CACHE_NOMAG)) {
		;
	}
	
	ret = NULL;
	
	return ret;
}

void slab_cache_free(slab_cache_t *cache, void *obj)
{
	ASSERT(cache != NULL);

	if (!FLAG_ON(cache->flags, SLAB_CACHE_NOMAG)) {
		;
	}
}

static int slab_cache_init(slab_cache_t *cache, const char *name, size_t size,
			   slab_ctor_t ctor, slab_dtor_t dtor, void *data,
			   int priority, int flags, int mmflag)
{
	int rc;

	ASSERT(size);

	strncpy(cache->name, name, SLAB_NAME_MAX);
	cache->name[SLAB_NAME_MAX - 1] = 0;
	
	cache->ctor = ctor;
	cache->dtor = dtor;
	cache->data = data;
	cache->priority = priority;

	DEBUG(DL_DBG, ("slab_cache_init: cache created %s\n", cache->name));
	
	return 0;
}

slab_cache_t *slab_cache_create(const char *name, size_t size, size_t align,
				slab_ctor_t ctor, slab_dtor_t dtor, void *data,
				int flags, int mmflag)
{
	slab_cache_t *cache;

	cache = slab_cache_alloc(&_slab_cache_cache, mmflag);
	if (!cache) {
		return NULL;
	}
	
	return cache;
}

void slab_cache_destroy(slab_cache_t *cache)
{
	list_del(&cache->link);
}

/* Initialize the slab allocator */
void init_slab()
{
	/* Initialize the cache for cache structures. */
	slab_cache_init(&_slab_cache_cache, "slab_cache_cache", sizeof(slab_cache_t),
			NULL, NULL, NULL, SLAB_METADATA_PRIORITY, 0, MM_BOOT);

	/* Initialize the magazine cache. */
	slab_cache_init(&_slab_mag_cache, "slab_cache_cache", sizeof(slab_magazine_t),
			NULL, NULL, NULL, SLAB_MAG_PRIORITY, SLAB_CACHE_NOMAG, MM_BOOT);

	/* Initialize other internal cache */
	slab_cache_init(&_slab_bufctl_cache, "slab_bufctl_cache", sizeof(slab_bufctl_t),
			NULL, NULL, NULL, SLAB_METADATA_PRIORITY, 0, MM_BOOT);
	slab_cache_init(&_slab_slab_cache, "slab_slab_cache", sizeof(slab_t),
			NULL, NULL, NULL, SLAB_METADATA_PRIORITY, 0, MM_BOOT);
}
