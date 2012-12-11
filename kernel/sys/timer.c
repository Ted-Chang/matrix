#include <types.h>
#include <stddef.h>
#include <string.h>
#include "sys/time.h"
#include "debug.h"
#include "hal/cpu.h"
#include "pit.h"
#include "timer.h"
#include "proc/thread.h"
#include "proc/sched.h"

extern void pit_delay(uint32_t usec);

useconds_t tmrs_clrtimer(struct list *head, struct timer *t)
{
	useconds_t prev_time = 0;
	struct timer *at;
	struct list *l;

	ASSERT((head != NULL) && (t != NULL));
	
	if (!LIST_EMPTY(head)) {
		at = LIST_ENTRY(&head->next, struct timer, link);
		prev_time = at->expire_time;
	}

	t->expire_time = TIMER_NEVER;

	/* Remove the timer from the active timer list */
	LIST_FOR_EACH(l, head) {
		at = LIST_ENTRY(l, struct timer, link);
		if (at == t) {
			list_del(l);
			break;
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
useconds_t tmrs_settimer(struct list *head, struct timer *t, useconds_t expire_time,
			 timer_func_t callback)
{
	struct timer *at;
	useconds_t old_head = 0;
	struct list *l;

	ASSERT((head != NULL) && (t != NULL));

	/* Clear the timer if it was already in the timer queue */
	tmrs_clrtimer(head, t);
	
	if (!LIST_EMPTY(head)) {
		at = LIST_ENTRY(&head->next, struct timer, link);
		old_head = at->expire_time;
	}

	/* Set the timer's variables */
	t->expire_time = expire_time + sys_time();
	t->func = callback;

	/* Add the timer to the active timer list, the next timer due is in front */
	LIST_FOR_EACH(l, head) {
		at = LIST_ENTRY(l, struct timer, link);
		if (expire_time < at->expire_time) {
			break;
		}
	}

	/* Keep the list ordered by due time */
	list_add(&t->link, l->prev);

	return old_head;
}

boolean_t tmrs_exptimers(struct list *head, useconds_t now)
{
	struct timer *t;
	struct list *l, *p;
	boolean_t preempt = FALSE;

	ASSERT(head != NULL);

	/* Use the current time to check the timers queue list for expired timers.
	 * Call the callback functions for all expired timers and deactive them.
	 * The caller is responsible for scheduling a new alarm if needed.
	 */
	LIST_FOR_EACH_SAFE(l, p, head) {
		t = LIST_ENTRY(l, struct timer, link);
		if (t->expire_time <= now) {
			list_del(&t->link);
			t->func(t->data);
			preempt = TRUE;
		} else {
			/* As our timer list is ordered, so just break if we have
			 * no timer expired
			 */
			break;
		}
	}

	return preempt;
}

void init_timer(struct timer *t, const char *name, void *data)
{
	ASSERT(t != NULL);
	
	LIST_INIT(&t->link);
	t->expire_time = TIMER_NEVER;
	t->func = NULL;
	t->data = NULL;
	t->data = data;
	strncpy(t->name, name, 15);
	t->name[15] = 0;
}

void set_timer(struct timer *t, useconds_t expire_time, timer_func_t callback)
{
	ASSERT(t != NULL);

	t->cpu = CURR_CPU;
	
	tmrs_settimer(&CURR_CPU->timers, t, expire_time, callback);

#ifdef _DEBUG_SCHED
	DEBUG(DL_DBG, ("name(%s), expire_time(%lld).\n", t->name, t->expire_time));
#endif	/* _DEBUG_SCHED */
}

void cancel_timer(struct timer *t)
{
	ASSERT(t != NULL);
	
	/* The timer pointed to by t was no longer needed, remove it from the active
	 * timer list. Always update the next timeout time by setting it to the front
	 * of the active timer list.
	 */
	tmrs_clrtimer(&CURR_CPU->timers, t);
}

void timer_delay(uint32_t usec)
{
	pit_delay(usec);
}

void timer_tick()
{
	useconds_t now;
	boolean_t prempt = FALSE;

	if (!CURR_CPU->timer_enabled) {
		return;
	}

	now = sys_time();
	
	prempt = tmrs_exptimers(&CURR_CPU->timers, now);
	if (prempt) {
		sched_reschedule(FALSE);
	}
}

