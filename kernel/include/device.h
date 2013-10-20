#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <types.h>

union dev_id {
	struct {
		uint16_t major;
		uint16_t minor;
	} part;
	uint32_t whole;
};
typedef union dev_id dev_id_t;

struct dev;

struct dev_ops {
	int (*open)();
	int (*close)(struct dev *d);
	int (*read)(struct dev *d, off_t off, size_t size);
	int (*write)(struct dev *d, off_t off, size_t size);
	void (*destroy)(struct dev *d);
};

struct dev {
	dev_id_t dev_id;	// ID for this device
	
	int ref_count;		// Reference count for this device
	int flags;		// Flags for this device
	void *data;		// Pointer to private data

	struct dev_ops *ops;
};

extern int dev_create(uint16_t major, int flags, void *ext, dev_t *dp);
extern int dev_open(dev_t dev_id, struct dev **dp);
extern int dev_close(struct dev *d);
extern int dev_read(struct dev *d, off_t off, size_t size);
extern int dev_write(struct dev *d, off_t off, size_t size);
extern void dev_destroy(dev_t dev_id);
extern void init_dev();

#endif	/* __DEVICE_H__ */
