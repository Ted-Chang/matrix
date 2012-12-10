#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

#include "list.h"
#include "hal/spinlock.h"

/* Semaphore structure definition */
struct semaphore {
	size_t count;
	struct spinlock lock;
	struct list threads;
	const char *name;
};
typedef struct semaphore semaphore_t;

extern void semaphore_down(struct semaphore *s);
extern void semaphore_up(struct semaphore *s, size_t count);
extern void semaphore_init(struct semaphore *s, const char *name, size_t initial);

#endif	/* __SEMAPHORE_H__ */
