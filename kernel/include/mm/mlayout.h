#ifndef __MLAYOUT_H__
#define __MLAYOUT_H__

/* Memory layout definition */
#define KERNEL_KMEM_START	0xC0000000
#define KERNEL_KMEM_SIZE	0x800000

/* Our kernel stack size is 8192 bytes */
#define KSTACK_SIZE		0x2000
/* Our user stack size is 65536 bytes */
#define USTACK_SIZE		0x4000

#endif	/* __MLAYOUT_H__ */
