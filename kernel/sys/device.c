#include <types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <limit.h>
#include "matrix/matrix.h"
#include "debug.h"
#include "mm/malloc.h"
#include "device.h"
#include "rtl/hashtable.h"
#include "mutex.h"

#define NR_MAX_MAJOR	0x1000

/* Definition of hash information */
struct dev_db_hash_info {
	const char *name;
	struct hashtable ht;
	uint32_t nr_hash_buckets;
	hashtable_hash_func hash_func;
	hashtable_compare_func compare_func;
	void *buckets_ptr;
};

struct dev_db_hash_info *_dev_db[NR_MAX_MAJOR];

static uint32_t dev_hash(void *key, uint32_t nr_buckets)
{
	uint32_t ret = ULONG_MAX;
	dev_t e = *((dev_t *)key);

	ret = e % nr_buckets;

	return ret;
}

static int dev_compare(void *key, void *entry)
{
	int ret;
	struct dev *d;
	dev_t dev;

	dev = *((dev_t *)key);
	d = (struct dev *)entry;

	if (dev == d->dev) {
		ret = 0;
	} else if (dev < d->dev) {
		ret = -1;
	} else {
		ret = 1;
	}

	return ret;
}

int dev_create(dev_t dev, int flags, void *ext, struct dev **dp)
{
	int rc = -1;
	uint32_t major;
	struct dev *device;
	struct dev_db_hash_info *hash_info = NULL;

	major = MAJOR(dev);

	/* Check whether the major was registered */
	hash_info = _dev_db[major];
	if (!hash_info) {
		DEBUG(DL_INF, ("invalid major %x.\n", major));
		rc = ENOENT;
		goto out;
	}

	if (FLAG_ON(flags, DEV_CREATE)) {
		/* We are creating a device */
		device = kmalloc(sizeof(*device), 0);
		if (!device) {
			rc = ENOMEM;
			goto out;
		}

		memset(device, 0, sizeof(*device));

		device->flags = 0;
		device->data = ext;
		device->ref_count = 1;	// Initial refcnt of the device is 1
		device->dev = dev;

		rc = hashtable_insert(&hash_info->ht, &dev, (void *)device);
		if (rc != 0) {
			goto out;
		}
		
		*dp = device;
	} else {
		/* We are opening a device */
		rc = hashtable_lookup(&hash_info->ht, &dev, (void **)&device);
		if (rc != 0) {
			rc = ENOENT;
			goto out;
		}
		
		device->ref_count++;
		*dp = device;
	}

 out:
	return rc;
}

int dev_close(struct dev *d)
{
	int rc = -1;

	if (!d) {
		rc = EINVAL;
		goto out;
	}

	if (!d->ops || !d->ops->close) {
		rc = EGENERIC;
		goto out;
	}
	
	rc = d->ops->close(d);

 out:
	return rc;
}

int dev_read(struct dev *d, off_t off, size_t size)
{
	int rc = -1;

	if (!d) {
		rc = EINVAL;
		goto out;
	}

	if (!d->ops || !d->ops->read) {
		rc = EGENERIC;
		goto out;
	}

	rc = d->ops->read(d, off, size);

 out:
	return rc;
}

int dev_write(struct dev *d, off_t off, size_t size)
{
	int rc = -1;

	if (!d) {
		rc = EINVAL;
		goto out;
	}

	if (!d->ops || !d->ops->write) {
		rc = EGENERIC;
		goto out;
	}

	rc = d->ops->write(d, off, size);

 out:
	return rc;
}

void dev_destroy(dev_t dev)
{
	ASSERT(dev != 0);
}

int dev_register(uint32_t major, const char *name)
{
	int rc = -1;
	uint32_t nr_buckets;
	struct dev_db_hash_info *hash_info = NULL;

	if (_dev_db[major]) {
		rc = EGENERIC;
		goto out;
	}

	hash_info = kmalloc(sizeof(*hash_info), 0);
	if (!hash_info) {
		rc = ENOMEM;
		goto out;
	}

	nr_buckets = 5;	// TODO: Tune the number of buckets
	hash_info->name = name;
	hash_info->nr_hash_buckets = nr_buckets;
	hash_info->buckets_ptr = kmalloc(sizeof(struct list) * nr_buckets, 0);
	if (!hash_info->buckets_ptr) {
		rc = ENOMEM;
		goto out;
	}

	hashtable_init(&hash_info->ht, hash_info->buckets_ptr,
		       nr_buckets, offsetof(struct dev, dev_link),
		       dev_hash, dev_compare, 0);

	_dev_db[major] = hash_info;
	rc = 0;

 out:
	if ((rc != 0) && (rc != EGENERIC)) {
		DEBUG(DL_WRN, ("register dev %s failed, err(%x).\n", rc));
		if (hash_info) {
			if (hash_info->buckets_ptr) {
				kfree(hash_info->buckets_ptr);
			}
			kfree(hash_info);
		}
	}
	
	return rc;
}

int dev_unregister(uint32_t major)
{
	int rc = -1;

	return rc;
}

void init_dev()
{
	memset(_dev_db, 0, sizeof(_dev_db));
}
