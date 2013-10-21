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
struct mutex _dev_db_lock;

static uint32_t dev_hash(void *key, uint32_t nr_buckets)
{
	uint32_t ret = ULONG_MAX;

	return ret;
}

static int dev_compare(void *key, void *entry)
{
	int ret = -1;

	return ret;
}

int dev_create(uint32_t major, int flags, void *ext, dev_t *dp)
{
	int rc = -1;
	dev_t devno;
	uint32_t minor;
	struct dev *device;

	if ((major == 0) || (dp == NULL)) {
		rc = EINVAL;
		goto out;
	}

	minor = 0;
	devno = MKDEV(major, minor);

	device = kmalloc(sizeof(*device), 0);
	if (!device) {
		rc = ENOMEM;
		goto out;
	}

	memset(device, 0, sizeof(*device));

	device->flags = flags;
	device->data = ext;
	device->ref_count = 1;	// Initial refcnt of the device is 1
	device->dev = devno;

	*dp = devno;
	
	rc = 0;

 out:
	return rc;
}

int dev_open(dev_t dev, struct dev **dp)
{
	int rc = -1;
	uint32_t major, minor;
	struct hashtable *ht = NULL;
	boolean_t lock_acquired = FALSE;

	major = MAJOR(dev);
	minor = MINOR(dev);

	if (major > NR_MAX_MAJOR) {
		rc = EINVAL;
		goto out;
	}

	mutex_acquire(&_dev_db_lock);
	lock_acquired = TRUE;

	if (!_dev_db[major]) {
		rc = ENOENT;
		goto out;
	}

	ht = &_dev_db[major]->ht;

	rc = hashtable_lookup(ht, &minor, (void **)dp);
	if (rc != 0) {
	 	rc = ENOENT;
	}

 out:
	if (lock_acquired) {
		mutex_release(&_dev_db_lock);
	}
	
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
	
	mutex_acquire(&_dev_db_lock);

	do {
		if (_dev_db[major]) {
			rc = EGENERIC;
			break;
		}

		_dev_db[major] = kmalloc(sizeof(struct dev_db_hash_info), 0);
		if (!_dev_db[major]) {
			rc = ENOMEM;
			break;
		}

		nr_buckets = 5;	// TODO: Tune the number of buckets
		_dev_db[major]->name = name;
		_dev_db[major]->nr_hash_buckets = nr_buckets;
		_dev_db[major]->buckets_ptr = kmalloc(sizeof(struct list) * nr_buckets, 0);
		if (!_dev_db[major]->buckets_ptr) {
			rc = ENOMEM;
			break;
		}

		hashtable_init(&_dev_db[major]->ht, _dev_db[major]->buckets_ptr,
				    nr_buckets, offsetof(struct dev, dev_link),
				    dev_hash, dev_compare, 0);
	} while (FALSE);

	if ((rc != 0) && (rc != EGENERIC)) {
		if (_dev_db[major]) {
			if (_dev_db[major]->buckets_ptr) {
				kfree(_dev_db[major]->buckets_ptr);
			}
			kfree(_dev_db[major]);
			_dev_db[major] = NULL;
		}
	}
	
	mutex_release(&_dev_db_lock);

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
	mutex_init(&_dev_db_lock, "devdb-mutex", 0);
}
