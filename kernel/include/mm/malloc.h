#ifndef __MALLOC_H__
#define __MALLOC_H__

#include <types.h>
#include <stddef.h>
#include "mm/mm.h"

void *kmalloc(size_t size, int mmflag);
void *kcalloc(size_t nmemb, size_t size, int mmflag);
void *krealloc(void *addr, size_t size, int mmflag);
void kfree(void *addr);

void init_malloc();

#endif	/* __MALLOC_H__ */
