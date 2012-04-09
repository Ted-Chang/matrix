#ifndef __MM_H__
#define __MM_H__

/* Allocation flags supported by all allocators */
#define MM_WAIT		(1<<0)	// Block until memory is available
#define MM_BOOT		(1<<1)	// Allocation is required for boot
#define MM_ZERO		(1<<2)	// Zero out allocated memory

#endif	/* __MM_H__ */
