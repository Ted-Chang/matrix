#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <types.h>
#include "task.h"

extern struct task *_curr_task;
extern struct task *_next_task;
extern struct task *_prev_task;

#endif	/* __GLOBAL_H__ */
