#ifndef __MM_H__
#define __MM_H__

/* Allocation flags supported by all allocators */
#define MM_WAIT		(1<<0)	// Wait until memory is available
#define MM_BOOT		(1<<1)	// Allocation is required for boot
#define MM_ZERO		(1<<2)	// Zero out the allocated memory
#define MM_ALIGN	(1<<3)	// Align to page bound

#endif	/* __MM_H__ */
