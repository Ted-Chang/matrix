#ifndef __MM_H__
#define __MM_H__

/* Allocation flags supported by all allocators */
#define MM_WAIT_F	(1<<0)	// Wait until memory is available
#define MM_BOOT_F	(1<<1)	// Allocation is required for boot
#define MM_ZERO_F	(1<<2)	// Zero out the allocated memory
#define MM_ALIGN_F	(1<<3)	// Align to page bound

#endif	/* __MM_H__ */
