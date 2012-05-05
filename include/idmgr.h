#ifndef __IDMGR_H__
#define __IDMGR_H__

#include "bitmap.h"

/* ID manager structure */
struct id_mgr {
	struct mutex lock;
	struct bitmap bitmap;
};

extern int32_t id_mgr_alloc(struct id_mgr *i);
extern void id_mgr_free(struct id_mgr *i, int32_t id);
extern void id_mgr_reserve(struct id_mgr *i, int32_t id);
extern status_t id_mgr_init(struct id_mgr *i, int32_t max, int mmflag);
extern void id_mgr_destroy(struct id_mgr *i);

#endif	/* __IDMGR_H__ */
