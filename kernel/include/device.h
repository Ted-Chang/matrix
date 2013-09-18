#ifndef __DEVICE_H__
#define __DEVICE_H__

struct dev;

struct dev_ops {
	int (*read)(struct dev *d);
	int (*write)(struct dev *d);
	void (*destroy)(struct dev *d);
};

struct dev {
	int ref_count;
	int flags;		// Flags for this device
	void *data;		// Pointer to private data
};

extern int dev_create(int flags, void *ext, struct dev **dp);
extern int dev_read(struct dev *d);
extern int dev_write(struct dev *d);
extern void dev_destroy(struct dev *d);
extern void init_dev();

#endif	/* __DEVICE_H__ */
