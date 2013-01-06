#ifndef __MLAYOUT_H__
#define __MLAYOUT_H__

/* Memory layout definition
 * +------------+
 * | 0xC0000000 | Kernel memory pool started address
 * +------------+
 * | 0x30000000 | User mode image loaded address
 * +------------+
 * | 0x20000000 | User thread stack address
 * +------------+
 */

/* Start address of the kernel memory pool */
#define KERNEL_KMEM_START	0xC0000000
/* Minimum size of the kernel memory pool */
#define KERNEL_KMEM_SIZE	0x400000

/* Our kernel stack size is 8192 bytes */
#define KSTACK_SIZE		0x2000
/* Our user stack size is 16384 bytes */
#define USTACK_SIZE		0x4000

#endif	/* __MLAYOUT_H__ */
