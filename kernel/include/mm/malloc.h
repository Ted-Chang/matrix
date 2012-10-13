#ifndef __MALLOC_H__
#define __MALLOC_H__

#include <types.h>
#include <stddef.h>
#include "mm/mm.h"

extern void *kmalloc(size_t size, int mmflag);
extern void *kcalloc(size_t nmemb, size_t size, int mmflag);
extern void *krealloc(void *addr, size_t size, int mmflag);
extern void kfree(void *addr);

extern void init_malloc();

#endif	/* __MALLOC_H__ */
