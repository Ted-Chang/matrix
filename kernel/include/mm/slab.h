#ifndef __SLAB_H__
#define __SLAB_H__

#include "list.h"
#include "mm/page.h"
#include "mm/mm.h"

/* Allocator limitation/settings */
#define SLAB_NAME_MAX		24	// Maximum slab cache name length

/* Slab constructor callback function */
typedef void (*slab_ctor_t)(void *obj);

/* Slab destructor callback function */
typedef void (*slab_dtor_t)(void *obj);

/* Slab cache structure */
struct slab_cache {
	size_t nr_slabs;		// Number of allocated slabs

	/* Slab lists/cache coloring settings */
	struct list slab_head;		// List of allocated slabs
	
	uint16_t color_next;		// Next cache color
	uint16_t color_max;		// Maximum cache color

	/* Cache settings */
	int flags;			// Cache behaviour flags
	size_t obj_size;		// Size of an object

	/* Callback functions */
	slab_ctor_t ctor;		// Object constructor function
	slab_dtor_t dtor;		// Object destructor function

	/* Debugging information */
	struct list link;		// Link to slab cache list
	char name[SLAB_NAME_MAX];	// Name of cache
};
typedef struct slab_cache slab_cache_t;

extern void *slab_cache_alloc(slab_cache_t *cache);
extern void slab_cache_free(slab_cache_t *cache, void *obj);
extern void slab_cache_init(slab_cache_t *c, const char *name,
			    size_t size, slab_ctor_t ctor,
			    slab_dtor_t dtor, int flags);
extern void slab_cache_delete(slab_cache_t *cache);
extern void init_slab();

#endif	/* __SLAB_H__ */
