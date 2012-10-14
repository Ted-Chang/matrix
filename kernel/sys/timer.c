#include <types.h>
#include <stddef.h>
#include "sys/time.h"
#include "matrix/debug.h"
#include "hal/cpu.h"
#include "pit.h"
#include "timer.h"

extern void pit_delay(uint32_t usec);

extern clock_t _next_timeout;

clock_t tmrs_clrtimer(struct list *head, struct timer *t)
{
	clock_t prev_time;
	struct timer *at;
	struct list *l;

	ASSERT(head != NULL);
	
	if (LIST_EMPTY(head)) {
		prev_time = 0;
	} else {
		at = LIST_ENTRY(&head->next, struct timer, link);
		prev_time = at->exp_time;
	}

	t->exp_time = TIMER_NEVER;

	/* Remove the timer from the active timer list */
	LIST_FOR_EACH(l, head) {
		at = LIST_ENTRY(l, struct timer, link);
		if (at == t) {
			list_del(&at->link);
		}
	}

	return prev_time;
}

/**
 * Activate a timer to run the callback function at exp_time. If the timer
 * is already in use it is first removed from the timers queue. Then it is
 * put in the list of active timers with the first to expire in front. The
 * caller is responsible for scheduling a new alarm for the timer if needed.
 */
clock_t tmrs_settimer(struct list *head, struct timer *t, clock_t exp_time,
		      timer_func_t callback)
{
	struct timer *at;
	clock_t old_head = 0;
	struct list *l;

	ASSERT(head != NULL);
	
	if (!LIST_EMPTY(head)) {
		at = LIST_ENTRY(&head->next, struct timer, link);
		old_head = at->exp_time;
	}

	/* Set the timer's variables */
	tmrs_clrtimer(head, t);
	t->exp_time = exp_time;
	t->timer_func = callback;

	/* Add the timer to the active timer list, the next timer due is in front */
	LIST_FOR_EACH(l, head) {
		at = LIST_ENTRY(l, struct timer, link);
		if (exp_time < at->exp_time) {
			break;
		}
	}

	/* Insert timer into the head of the timer list */
	list_add(&t->link, head);

	return old_head;
}

void tmrs_exptimers(struct list *head, clock_t now)
{
	struct timer *t;
	struct list *l, *p;

	ASSERT(head != NULL);

	/* Use the current time to check the timers queue list for expired timers.
	 * Call the callback functions for all expired timers and deactive them.
	 * The caller is responsible for scheduling a new alarm if needed.
	 */
	LIST_FOR_EACH_SAFE(l, p, head) {
		t = LIST_ENTRY(l, struct timer, link);
		if (t->exp_time <= now) {
			list_del(&t->link);
			t->exp_time = TIMER_NEVER;
			t->timer_func(t);
		}
	}
}

void init_timer(struct timer *t)
{
	ASSERT(t != NULL);
	
	t->exp_time = TIMER_NEVER;
	LIST_INIT(&t->link);
}

void set_timer(struct timer *t, clock_t exp_time, timer_func_t callback)
{
	struct timer *at;
	
	ASSERT(t != NULL);
	
	tmrs_settimer(&CURR_CPU->timers, t, exp_time, callback);
	ASSERT(!LIST_EMPTY(&CURR_CPU->timers));

	at = LIST_ENTRY(&CURR_CPU->timers.next, struct timer, link);
	_next_timeout = at->exp_time;
}

void cancel_timer(struct timer *t)
{
	ASSERT(t != NULL);
	
	/* The timer pointed to by t was no longer needed, remove it from the active
	 * timer list. Always update the next timeout time by setting it to the front
	 * of the active timer list.
	 */
	tmrs_clrtimer(&CURR_CPU->timers, t);
	if (LIST_EMPTY(&CURR_CPU->timers)) {
		_next_timeout = TIMER_NEVER;
	} else {
		struct timer *at;

		at = LIST_ENTRY(&CURR_CPU->timers.next, struct timer, link);
		_next_timeout = at->exp_time;
	}
}

void timer_delay(uint32_t usec)
{
	pit_delay(usec);
}
