#include <types.h>
#include <stddef.h>
#include <string.h>
#include "atomic.h"
#include "hal/hal.h"
#include "hal/lirq.h"
#include "cpu.h"
#include "status.h"
#include "matrix/debug.h"
#include "mm/kmem.h"
#include "clock.h"

/* Per-CPU scheduler structure */
struct scheduler {
	struct spinlock lock;

	struct timer timer;
	size_t total;
};

/* Total number of running or ready threads across all CPUs */
static atomic_t _nr_running_threads = 0;

static boolean_t sched_timer_func(void *ctx)
{
	kprintf("sched_timer_func\n");
}

/* Initialize the scheduler for the current cpu */
void init_sched_per_cpu()
{
	status_t rc;
	int i, j;

	/* Create the per-CPU information structure */
	CURR_CPU->sched = kmem_alloc(sizeof(struct scheduler));

	/* Create the idle thread */
	//...

	/* Create the preemption timer */
	init_timer(&CURR_CPU->sched->timer, "sched_timer", 0, TIMER_PERIODIC);

	/* Initialize the schedule queues */
	for (i = 0; i < 2; i++) {
		;
	}
}

void sched_enter()
{
	set_timer(&CURR_CPU->sched->timer, 1000000, sched_timer_func, NULL);
}

void init_sched()
{
	/* Create the reaper thread */
	;
}
