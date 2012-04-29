#ifndef __TIMER_H__
#define __TIMER_H__

#define PIT_BASE_FREQUENCY	1193182L	// clock frequency for timer in PC and AT

#define TIMER_NEVER		(-1)

struct timer;

typedef void (*timer_func_t)(struct timer *tp);

struct timer {
	struct timer *next;
	clock_t exp_time;
	timer_func_t timer_func;
	void *timer_ctx;
};

void init_timer(struct timer *t);
void set_timer(struct timer *t, clock_t exp_time, timer_func_t callback);
void cancel_timer(struct timer *t);

#endif	/* __TIMER_H__ */
