#ifndef __TIMER_H__
#define __TIMER_H__

#define TIMER_NEVER	(-1)

struct timer;

typedef void (*timer_func_t)(struct timer *tp);

struct timer {
	struct timer *next;
	clock_t exp_time;
	timer_func_t timer_func;
	void *timer_ctx;
};

extern void init_clock();
extern void stop_clock();
extern clock_t get_uptime();
extern u_long read_clock();
extern void init_timer(struct timer *t);
extern void set_timer(struct timer *t, clock_t exp_time, timer_func_t callback);
extern void cancel_timer(struct timer *t);
extern void timer_delay(uint32_t msec);

#endif	/* __TIMER_H__ */
