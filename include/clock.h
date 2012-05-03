#ifndef __CLOCK_H__
#define __CLOCK_H__

#include "list.h"

/* Seconds to microseconds */
#define SECS2USECS(secs)	((useconds_t)secs * 1000000)

struct timer_dev {
	const char *name;

	enum {
		TIMER_DEV_PERIODIC,
		TIMER_DEV_ONESHOT,
	} type;

	/* Enable the device (for periodic timers) */
	void (*enable)();
	/* Disable the device (for periodic timers) */
	void (*disable)();
	/* Setup the next tick (for one-shot timers)*/
	void (*prepare)(useconds_t us);
};

typedef boolean_t (*timer_func_t)(void *ctx);

struct timer {
	struct list header;	// Link to the timer list

	useconds_t target;	// Time at which the timer will fire
	struct cpu *cpu;	// The CPU that the timer was started on
	timer_func_t func;	// Callback function of this timer
	void *ctx;		// Context for this timer
	uint32_t flags;		// Flags for the timer
	uint32_t mode;		// Mode of the timer
	useconds_t initial;	// Initial time (for periodic timers)
	const char *name;	// Name of the timer
};

extern useconds_t platform_time_from_cmos();
extern useconds_t time_to_unix(uint32_t year, uint32_t mon, uint32_t day,
			       uint32_t hour, uint32_t min, uint32_t sec);
extern useconds_t sys_time();
extern boolean_t do_clocktick();

extern void set_timer_dev(struct timer_dev *dev);

extern void init_timer(struct timer *t, const char *name, uint32_t flags);
extern void set_timer(struct timer *t, useconds_t exp_time, timer_func_t func, void *ctx);
extern void cancel_timer(struct timer *t);

extern void init_clock();

#endif	/* __CLOCK_H__ */
