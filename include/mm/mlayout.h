#ifndef __MLAYOUT_H__
#define __MLAYOUT_H__

/* Memory layout definitions */
#define USER_BASE		0x00000000	// User address space base
#define USER_SIZE		0xC0000000	// User address space size
#define KERNEL_BASE		0xC0000000	// Kernel address space base
#define KERNEL_SIZE		0x40000000	// Kernel address space size (1GB)
#define KERNEL_PMAP_BASE	0xC0000000	// Physical map area base
#define KERNEL_PMAP_SIZE	0x20000000	// Physical map area size
#define KERNEL_PMAP_OFFSET	0x00000000	// Physical map area offset
#define KERNEL_KMEM_BASE	0xE0000000	// Kernel allocation area base
#define KERNEL_KMEM_SIZE	0x00100000	// Kernel allocation size
#define KERNEL_VIRT_BASE	0x
#define KERNEL_MODULE_BASE	0x
#define KERNEL_MODULE_SIZE	0x

/* Stack size for kernel and user */
#define KSTK_SIZE		0x2000		// Kernel stack size (8KB)
#define USTK_SIZE		0x20000		// User stack size

#endif	/* __MLAYOUT_H__ */
