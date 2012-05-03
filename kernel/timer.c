#include <types.h>
#include <stddef.h>
#include "hal/spinlock.h"
#include "matrix/debug.h"
#include "cpu.h"
#include "clock.h"

extern void timer_dev_prepare(struct timer *t);

extern struct timer_dev *_timer_dev;

static void tmrs_settimer(struct timer *t)
{
	struct timer *tmr;
	struct list *pos;
	
	ASSERT(list_empty(&t->header));

	/* Work out the absolute completion time */
	t->target = sys_time() + t->initial;

	/* Insert the timer into the ordered list which key is expiration time */
	pos = CURR_CPU->timers.next;
	while (pos != &CURR_CPU->timers) {
		tmr = LIST_ENTRY(pos, struct timer, header);
		if (tmr->target > t->target)
			break;
		pos = pos->next;
	}

	list_add(&t->header, pos->prev);
}

void init_timer(struct timer *t, const char *name, uint32_t flags)
{
	LIST_INIT(&t->header);
	t->name = name;
	t->flags = flags;
}

void set_timer(struct timer *t, useconds_t exp_time, timer_func_t func, void *ctx)
{
	struct timer *tmr;
	
	t->cpu = CURR_CPU;
	t->initial = exp_time;
	t->func = func;
	t->ctx = ctx;

	spinlock_acquire(&CURR_CPU->timer_lock);

	/* Add the timer to the list */
	tmrs_settimer(t);

	switch (_timer_dev->type) {
	case TIMER_DEV_ONESHOT:
		/* If the new timer is at the beginning of the list, then it
		 * has the shortest remaining time, so we need to adjust the
		 * device to tick for it.
		 */
		tmr = LIST_ENTRY(CURR_CPU->timers.next, struct timer, header);
		if (t == tmr) {
			timer_dev_prepare(t);
		}
		break;
	case TIMER_DEV_PERIODIC:
		break;
	}
	
	spinlock_release(&CURR_CPU->timer_lock);
}

void cancel_timer(struct timer *t)
{
	struct timer *first;

	if (!list_empty(&t->header)) {
		ASSERT(t->cpu);

		spinlock_acquire(&t->cpu->timer_lock);

		first = LIST_ENTRY(t->cpu->timers.next, struct timer, header);
		list_del(&t->header);
		
		if (t->cpu == CURR_CPU) {
			switch (_timer_dev->type) {
			case TIMER_DEV_ONESHOT:
				if (first == t && !list_empty(&CURR_CPU->timers)) {
					first = LIST_ENTRY(CURR_CPU->timers.next,
							   struct timer, header);
					timer_dev_prepare(first);
				}
				break;
			case TIMER_DEV_PERIODIC:
				break;
			}
		}
		
		spinlock_release(&t->cpu->timer_lock);
	}
}
