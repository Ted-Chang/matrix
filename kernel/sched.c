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

/* Initialize the scheduler for the current cpu */
void init_sched_per_cpu()
{
	status_t rc;
	int i, j;

	/* Create the per-CPU information structure */
	CURR_CPU->sched = kmem_alloc(sizeof(struct scheduler));

	/* Create the idle thread */
	rc = STATUS_SUCCESS;

	/* Create the preemption timer */
	init_timer(&CURR_CPU->sched->timer, "sched_timer", 0);

	/* Initialize queues */
	for (i = 0; i < 2; i++) {
		;
	}
}

void init_sched()
{
	;
}
