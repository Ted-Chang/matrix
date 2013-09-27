#ifndef __DEVICE_H__
#define __DEVICE_H__

struct dev;

struct dev_ops {
	int (*open)();
	int (*close)(struct dev *d);
	int (*read)(struct dev *d, off_t off, size_t size);
	int (*write)(struct dev *d, off_t off, size_t size);
	void (*destroy)(struct dev *d);
};

struct dev {
	int ref_count;
	int flags;		// Flags for this device
	void *data;		// Pointer to private data

	struct dev_ops *ops;
};

extern int dev_create(int flags, void *ext, struct dev **dp);
extern int dev_open();
extern int dev_close(struct dev *d);
extern int dev_read(struct dev *d, off_t off, size_t size);
extern int dev_write(struct dev *d, off_t off, size_t size);
extern void dev_destroy(struct dev *d);
extern void init_dev();

#endif	/* __DEVICE_H__ */
