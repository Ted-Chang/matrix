#include <types.h>
#include <stddef.h>
#include <time.h>
#include "timer.h"

extern clock_t _next_timeout;
struct timer *_active_timers;

clock_t tmrs_clrtimer(struct timer **list, struct timer *t)
{
	struct timer **at;
	clock_t prev_time;

	if (*list)
		prev_time = (*list)->exp_time;
	else
		prev_time = 0;

	t->exp_time = TIMER_NEVER;

	/* Remove the timer from the active timer list */
	for (at = list; (*at) != NULL; at = &((*at)->next)) {
		if (*at == t)
			*at = t->next;
	}

	return prev_time;
}

/**
 * Activate a timer to run the callback function at exp_time. If the timer
 * is already in use it is first removed from the timers queue. Then it is
 * put in the list of active timers with the first to expire in front. The
 * caller is responsible for scheduling a new alarm for the timer if needed.
 */
clock_t tmrs_settimer(struct timer **list, struct timer *t, clock_t exp_time,
		      timer_func_t callback)
{
	struct timer **at;
	clock_t old_head = 0;

	if (*list)
		old_head = (*list)->exp_time;

	/* Set the timer's variables */
	tmrs_clrtimer(list, t);
	t->exp_time = exp_time;
	t->timer_func = callback;

	/* Add the timer to the active timer list, the next timer due is in front */
	for (at = list; *at != NULL; at = &((*at)->next)) {
		if (exp_time < (*at)->exp_time)
			break;
	}

	t->next = *at;
	*at = t;

	return old_head;
}

void tmrs_exptimers(struct timer **list, clock_t now)
{
	struct timer *t;

	/* Use the current time to check the timers queue list for expired timers.
	 * Call the callback functions for all expired timers and deactive them.
	 * The caller is responsible for scheduling a new alarm if needed.
	 */
	while (((t = *list) != NULL) && (t->exp_time <= now)) {
		*list = t->next;
		t->exp_time = TIMER_NEVER;
		t->timer_func(t);
	}
}

void init_timer(struct timer *t)
{
	t->exp_time = TIMER_NEVER;
	t->next = NULL;
}

void set_timer(struct timer *t, clock_t exp_time, timer_func_t callback)
{
	tmrs_settimer(&_active_timers, t, exp_time, callback);
	_next_timeout = _active_timers->exp_time;
}

void cancel_timer(struct timer *t)
{
	/* The timer pointed to by t was no longer needed, remove it from the active
	 * timer list. Always update the next timeout time by setting it to the front
	 * of the active timer list.
	 */
	tmrs_clrtimer(&_active_timers, t);
	_next_timeout = (_active_timers == NULL) ?
		TIMER_NEVER : _active_timers->exp_time;
}
