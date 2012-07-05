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
#include "proc/thread.h"

/* Number of priority levels */
#define NR_PRIORITIES		32

/* Timeslice to give to threads in micro seconds */
#define THREAD_TIMESLICE	10000

/* Schedule queue structure */
struct sched_queue {
	uint32_t bitmap;			// Bitmap of queues with data
	struct list threads[NR_PRIORITIES];	// Queues of runnable threads
};

/* Per-CPU scheduler structure */
struct scheduler {
	struct spinlock lock;

	struct thread *prev_thread;		// Previously executed thread
	struct thread *idle_thread;		// Thread scheduled when no other threads runnable
	
	struct timer timer;			// Preemption timer
	struct sched_queue *active;
	struct sched_queue *expired;
	struct sched_queue queues[2];		// Active and expired queues
	size_t total;				// Total running or ready thread count
};

/* Total number of running or ready threads across all CPUs */
static atomic_t _nr_running_threads = 0;

static boolean_t sched_timer_func(void *ctx)
{
	kprintf("sched_timer_func\n");

	return TRUE;
}

static INLINE void sched_queue_enqueue(struct sched_queue *q, struct thread *t)
{
	list_add_tail(&t->runq_link, &q->threads[t->curr_priority]);
	q->bitmap |= (1 << t->curr_priority);
}

static INLINE void sched_queue_dequeue(struct sched_queue *q, struct thread *t)
{
	list_del(&t->runq_link);
	if (list_empty(&q->threads[t->curr_priority]))
		q->bitmap &= ~(1 << t->curr_priority);
}

static INLINE void sched_calculate_priority(struct thread *t)
{
	int priority;

	priority = 5;

	t->max_priority = t->curr_priority = priority;
}

void sched_insert_thread(struct thread *t)
{
	struct scheduler *sched;

	ASSERT(t->state == THREAD_READY);

	/* If we have been newly created, we will not have a priority set.
	 * Calculate the maximum priority for the thread.
	 */
	if (t->max_priority < 0)
		sched_calculate_priority(t);

	t->cpu = CURR_CPU;

	sched = t->cpu->sched;
	spinlock_acquire(&sched->lock);

	sched_queue_enqueue(sched->active, t);
	sched->total++;
	
	spinlock_release(&sched->lock);
}

static void sched_idle_thread(void *arg)
{
	/* We run the loop with interrupts disabled. The cpu_idle() function
	 * is expected to re-enable interrupts as required.
	 */
	irq_disable();

	while (TRUE) {
		;
	}
}

/* Initialize the scheduler for the current cpu */
void init_sched_per_cpu()
{
	status_t rc;
	int i, j;

	/* Create the per-CPU information structure */
	CURR_CPU->sched = kmem_alloc(sizeof(struct scheduler));
	spinlock_init(&CURR_CPU->sched->lock, "sched_lock");
	CURR_CPU->sched->total = 0;
	CURR_CPU->sched->active = &CURR_CPU->sched->queues[0];
	CURR_CPU->sched->expired = &CURR_CPU->sched->queues[1];

	/* Create the idle thread */
	rc = create_thread(NULL, 0, sched_idle_thread, NULL, NULL);
	if (rc != STATUS_SUCCESS) {
		PANIC("Could not create idle thread.");
	}

	CURR_CPU->sched->prev_thread = NULL;

	/* Create the preemption timer */
	init_timer(&CURR_CPU->sched->timer, "sched_timer", 0, TIMER_PERIODIC);

	/* Initialize the schedule queues */
	for (i = 0; i < 2; i++) {
		CURR_CPU->sched->queues[i].bitmap = 0;
		for (j = 0; j < NR_PRIORITIES; j++) {
			LIST_INIT(&CURR_CPU->sched->queues[i].threads[j]);
		}
	}
}

void sched_enter()
{
	set_timer(&CURR_CPU->sched->timer, 10000000, sched_timer_func, NULL);

	DEBUG(DL_DBG, ("sched_enter() done!\n"));
}

void init_sched()
{
	/* Create the reaper thread */
	;
}
