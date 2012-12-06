#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#include <types.h>
#include "matrix/matrix.h"

/* Atomic variable type (32 bit) */
typedef volatile int32_t atomic_t;

/* Atomic variable type (64 bit) */
typedef volatile int64_t atomic64_t;

static INLINE int32_t atomic_add(atomic_t *var, int32_t val)
{
	asm volatile("lock xaddl %0, %1" : "+r"(val), "+m"(*var) :: "memory");
	return val;
}

static INLINE int32_t atomic_sub(atomic_t *var, int32_t val)
{
	return atomic_add(var, -val);
}

static INLINE int32_t atomic_inc(atomic_t *var)
{
	return atomic_add(var, 1);
}

static INLINE int32_t atomic_dec(atomic_t *var)
{
	return atomic_sub(var, 1);
}

static INLINE int atomic_tas(atomic_t *var, int32_t test, int32_t val)
{
	int res;

	asm volatile("lock; \
		      cmpxchgl %3, %1; \
		      setz %%al; \
		      movzx %%al, %0"
		      : "=a"(res), "+m"(*var)
		      : "0"(test), "r"(val));

	return res;
}

#if _AMD64_

static INLINE int64_t atomic_add64(atomic64_t *var, int64_t val)
{
	asm volatile("lock xaddq %0, %1" : "+r"(val), "+m"(*var) :: "memory");
	return val;
}

static INLINE int64_t atomic_sub64(atomic64_t *var, int64_t val)
{
	return atomic_add64(var, -val);
}

static INLINE int64_t atomic_inc64(atomic64_t *var)
{
	return atomic_add64(var, 1);
}

static INLINE int64_t atomic_dec64(atomic64_t *var)
{
	return atomic_sub64(var, 1);
}

static INLINE int atomic_tas64(atomic64_t *var, int64_t test, int64_t val)
{
	int res;

	asm volatile("lock; \
		      cmpxchgq %3, %1; \
		      setz %%al; \
		      movzx %%al, %0"
		      : "=a"(res), "+m"(*var)
		      : "0"(test), "r"(val));
	
	return res;
}

#endif

#endif	/* __ATOMIC_H__ */
