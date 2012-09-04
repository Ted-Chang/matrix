#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <types.h>
#include "proc/process.h"

extern struct process *_next_proc;
extern struct process *_prev_proc;

extern struct process *_ready_head[NR_SCHED_QUEUES];
extern struct process *_ready_tail[NR_SCHED_QUEUES];


#endif	/* __GLOBAL_H__ */
