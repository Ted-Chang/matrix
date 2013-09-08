#ifndef __TIMER_H__
#define __TIMER_H__

#include "matrix/matrix.h"
#include "list.h"

#define TIMER_NEVER	(-1)

struct timer;

typedef void (*timer_func_t)(void *ctx);

struct timer {
	struct list link;		// Link to timers list
	struct core *core;		// CORE that the timer was started on
	useconds_t expire_time;		// Time at which the timer will fire
	timer_func_t func;		// Function to call when the timer expires
	int flags;			// Flags for the timer
	void *ctx;			// Argument to pass to timer handler
	char name[16];			// Name for the timer
};
typedef struct timer timer_t;

/* Flags for timer */
#define TIMER_SCHED	(1<<0)		// Scheduler timer, reserved for system use

extern void init_timer(struct timer *t, const char *name, int flags);
extern void set_timer(struct timer *t, useconds_t expire_time,
		      timer_func_t callback, void *ctx);
extern void cancel_timer(struct timer *t);
extern void timer_delay(uint32_t us);
extern void timer_tick();

#endif	/* __TIMER_H__ */
