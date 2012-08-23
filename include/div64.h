#ifndef __DIV64_H__
#define __DIV64_H__

#if BITS_PER_LONG == 64

#define do_div(n, base) ({		\
	uint32_t __base = (base);	\
	uint32_t __rem;			\
	__rem = ((uint64_t)(n)) % __base;	\
	(n) = ((uint64_t)(n)) / __base;		\
	__rem;					\
})

#elif BITS_PER_LONG == 32

extern uint32_t __div64_32(uint64_t *dividend, uint32_t divisor);

#define do_div(n, base) ({	  \
	uint32_t __base = (base); \
	uint32_t __rem;		  \
	if ((n) >> 32 == 0) {			\
		__rem = (uint32_t)(n) % __base; \
		(n) = (uint32_t)(n) / __base;	\
	} else					  \
		__rem = __div64_32(&(n), __base); \
	__rem;					  \
})

#endif	/* BITS_PER_LONG */

#endif	/* __DIV64_H__ */
