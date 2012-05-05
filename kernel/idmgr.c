#include <types.h>
#include <stddef.h>
#include "status.h"
#include "mutex.h"
#include "matrix/debug.h"
#include "idmgr.h"

int32_t id_mgr_alloc(struct id_mgr *i)
{
	int32_t id;

	mutex_acquire(&i->lock);

	id = bitmap_ffz(&i->bitmap);
	if (id < 0) {
		mutex_release(&i->lock);
		return -1;
	}

	bitmap_set(&i->bitmap, id);

	mutex_release(&i->lock);

	return id;
}

void id_mgr_free(struct id_mgr *i, int32_t id)
{
	mutex_acquire(&i->lock);

	ASSERT(bitmap_test(&i->bitmap, id));
	bitmap_clear(&i->bitmap, id);

	mutex_release(&i->lock);
}

void id_mgr_reserve(struct id_mgr *i, int32_t id)
{
	mutex_acquire(&i->lock);

	ASSERT(!bitmap_test(&i->bitmap, id));
	bitmap_set(&i->bitmap, id);

	mutex_release(&i->lock);
}

status_t id_mgr_init(struct id_mgr *i, int32_t max, int mmflag)
{
	mutex_init(&i->lock, "id_mgr_lock", 0);

	return bitmap_init(&i->bitmap, max + 1, NULL, mmflag);
}

void id_mgr_destroy(struct id_mgr *i)
{
	bitmap_destroy(&i->bitmap);
}
