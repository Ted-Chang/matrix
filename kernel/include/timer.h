#ifndef __TIMER_H__
#define __TIMER_H__

#include "matrix/matrix.h"
#include "list.h"

#define TIMER_NEVER	(-1)

struct timer;

typedef void (*timer_func_t)(void *ctx);

struct timer {
	struct list link;		// Link to timers list
	
	struct cpu *cpu;		// CPU that the timer was started on

	useconds_t expire_time;		// Time at which the timer will fire

	timer_func_t func;		// Function to call when the timer expires

	void *data;			// Argument to pass to timer handler

	char name[16];			// Name for the timer
};
typedef struct timer timer_t;

extern void init_clock();
extern void stop_clock();
extern void init_timer(struct timer *t, const char *name, void *data);
extern void set_timer(struct timer *t, useconds_t expire_time, timer_func_t callback);
extern void cancel_timer(struct timer *t);
extern void timer_delay(uint32_t msec);

#endif	/* __TIMER_H__ */
