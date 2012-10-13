#ifndef __SLAB_H__
#define __SLAB_H__

#include "list.h"
#include "mm/page.h"
#include "mm/mm.h"

/* Allocator limitation/settings */
#define SLAB_NAME_MAX		24	// Maximum slab cache name length
#define SLAB_MAGAZINE_SIZE	16	// Initial magazine size (resizing currently not supported)

/* Slab constructor callback function */
typedef void (*slab_ctor_t)(void *obj, void *data);

/* Slab destructor callback function */
typedef void (*slab_dtor_t)(void *obj, void *data);

/* Slab cache structure */
struct slab_cache {
	size_t nr_slabs;
	size_t color_next;		// Next cache color
	size_t color_max;		// Maximum cache color

	/* Cache settings */
	int flags;			// Cache behaviour flags
	size_t slab_size;		// Size of a slab
	size_t obj_size;		// Size of an object
	size_t nr_objs;			// Number of objects per slab
	size_t align;			// Required alignment of each object

	/* Callback functions */
	slab_ctor_t ctor;		// Object constructor function
	slab_dtor_t dtor;		// Object destructor function
	void *data;			// Data to pass to helper function
	int priority;			// Reclaim priority

	/* Debugging information */
	struct list link;		// Link to slab cache list
	char name[SLAB_NAME_MAX];	// Name of cache
};
typedef struct slab_cache slab_cache_t;

/* Slab cache flags */
#define SLAB_CACHE_NOMAG	(1<<0)	// Disable the magazine layer
#define SLAB_CACHE_LARGE	(1<<1)	// Cache is large object cache
#define SLAB_CACHE_LATEMAG	(1<<2)	// Internal, do not set


extern void *slab_cache_alloc(slab_cache_t *cache, int mmflag);
extern void slab_cache_free(slab_cache_t *cache, void *obj);
extern slab_cache_t *slab_cache_create(const char *name, size_t size, size_t align,
				       slab_ctor_t ctor, slab_dtor_t dtor,
				       void *data, int flags, int mmflag);
extern void slab_cache_destroy(slab_cache_t *cache);
extern void init_slab();

#endif	/* __SLAB_H__ */
