#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/matrix.h"
#include "mm/mm.h"
#include "mm/kmem.h"
#include "mm/malloc.h"
#include "mm/slab.h"
#include "matrix/debug.h"

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

	/* Allocate a new slab */
	addr = kmem_alloc(cache->slab_size, 0);
	if (!addr) {
		return NULL;
	}
	
	slab = NULL;

	cache->nr_slabs++;

	slab->base = addr;
	slab->ref_count = 0;
	LIST_INIT(&slab->link);
	slab->parent = cache;
	
	return slab;
}

void *slab_cache_alloc(slab_cache_t *cache, int mmflag)
{
	void *ret;

	ret = NULL;
	
	return ret;
}

void slab_cache_free(slab_cache_t *cache, void *obj)
{
	;
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
