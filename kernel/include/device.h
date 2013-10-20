#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <types.h>

#define MINORBITS	20
#define MINORMASK	((1U << MINORBITS) - 1)
#define MAJOR(dev)	((unsigned int)(dev) >> MINORBITS)
#define MINOR(dev)	((unsigned int)(dev) & MINORMASK)
#define MKDEV(ma, mi)	((ma) << MINORBITS | (mi))

struct dev;

struct dev_ops {
	int (*open)();
	int (*close)(struct dev *d);
	int (*read)(struct dev *d, off_t off, size_t size);
	int (*write)(struct dev *d, off_t off, size_t size);
	void (*destroy)(struct dev *d);
};

struct dev {
	dev_t dev;		// ID for this device
	
	int ref_count;		// Reference count for this device
	int flags;		// Flags for this device
	void *data;		// Pointer to private data

	struct dev_ops *ops;
};

extern int dev_create(uint32_t major, int flags, void *ext, dev_t *dp);
extern int dev_open(dev_t dev, struct dev **dp);
extern int dev_close(struct dev *d);
extern int dev_read(struct dev *d, off_t off, size_t size);
extern int dev_write(struct dev *d, off_t off, size_t size);
extern void dev_destroy(dev_t dev);
extern void init_dev();

#endif	/* __DEVICE_H__ */
