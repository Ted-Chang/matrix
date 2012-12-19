#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/matrix.h"
#include "mm/slab.h"
#include "debug.h"
#include "kd.h"
#include "mutex.h"
#include "proc/thread.h"
#include "proc/process.h"

uint32_t _value = 0;
struct mutex _mutex;

static void unit_test_thread(void *ctx)
{
	struct mutex *m;

	m = (struct mutex *)ctx;

	DEBUG(DL_DBG, ("unit test thread(%s) is running.\n", CURR_THREAD->name));

	while (_value < 0x00000FFF) {
		mutex_acquire(m);
		_value++;
		DEBUG(DL_DBG, ("thread(%s) mutex(%p:%s) acquired, value(%x).\n",
			       CURR_THREAD->name, m, m->name, _value));
		mutex_release(m);
	}
}

int unit_test(uint32_t round)
{
	int i, r, rc = 0;
	slab_cache_t ut_cache;
	void *obj[4];
	struct spinlock lock;

	/* Spinlock test */
	spinlock_init(&lock, "ut-lock");
	spinlock_acquire(&lock);
	spinlock_release(&lock);
	DEBUG(DL_DBG, ("spinlock test finished.\n"));
	

	/* Create a slab cache to test the slab allocator */
	r = 0;
	memset(obj, 0, sizeof(obj));
	slab_cache_init(&ut_cache, "ut-cache", 256, NULL, NULL, 0);
	while (TRUE) {
		for (i = 0; i < 4; i++) {
			ASSERT(obj[i] == NULL);
			obj[i] = slab_cache_alloc(&ut_cache);
			if (!obj[i]) {
				DEBUG(DL_INF, ("slab_cache_alloc failed.\n"));
				goto out;
			} else {
				memset(obj[i], 0, 256);
			}
		}

		for (i = 0; i < 4; i++) {
			if (obj[i]) {
				slab_cache_free(&ut_cache, obj[i]);
				obj[i] = NULL;
			}
		}
		
		r++;
		if (r > round) {
			break;
		}
	}
	DEBUG(DL_DBG, ("slab cache test finished with round %d.\n", round));


	/* Create two kernel thread to do the mutex test */
	mutex_init(&_mutex, "ut-mutex", 0);
	rc = thread_create("unit-test1", NULL, 0, unit_test_thread, &_mutex, NULL);
	if (rc != 0) {
		DEBUG(DL_DBG, ("thread_create ut1 failed, err(%d).\n", rc));
		goto out;
	}
	rc = thread_create("unit-test2", NULL, 0, unit_test_thread, &_mutex, NULL);
	if (rc != 0) {
		DEBUG(DL_DBG, ("thread_create ut2 failed, err(%d).\n", rc));
		goto out;
	}
	DEBUG(DL_DBG, ("mutex test finished.\n"));


 out:
	for (i = 0; i < 4; i++) {
		if (obj[i]) {
			slab_cache_free(&ut_cache, obj[i]);
			obj[i] = NULL;
		}
	}
	slab_cache_delete(&ut_cache);
	
	return rc;
}
