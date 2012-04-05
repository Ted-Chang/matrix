#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <types.h>
#include "proc/task.h"

extern struct task *_curr_task;
extern struct task *_next_task;
extern struct task *_prev_task;

extern struct task *_ready_head[NR_SCHED_QUEUES];
extern struct task *_ready_tail[NR_SCHED_QUEUES];


#endif	/* __GLOBAL_H__ */
