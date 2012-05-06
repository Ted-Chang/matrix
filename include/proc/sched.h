#ifndef __SCHED_H__
#define __SCHED_H__

#include "proc/thread.h"

extern void sched_insert_thread(struct thread *t);

extern void sched_enter();
extern void init_sched_per_cpu();
extern void init_sched();

#endif	/* __SCHED_H__ */
