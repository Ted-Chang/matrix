#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "mm/kmem.h"
#include "mm/malloc.h"
#include "mm/slab.h"

struct slab {
	struct list link;	// Link to appropriate slab list in cache
	
	void *base;		// Base address of allocation
	size_t ref_count;	// Reference count
	size_t color;		// Color of the slab
	slab_cache_t *parent;	// Cache containing the slab
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
}

static slab_t *slab_create()
{
	slab_t *slab;

	slab = NULL;
	
	return slab;
}

void *slab_cache_alloc(slab_cache_t *cache, int mmflag)
{
	void *ret;

	return ret;
}

void slab_cache_free(slab_cache_t *cache, void *obj)
{
	;
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

void init_slab()
{
	;
}
