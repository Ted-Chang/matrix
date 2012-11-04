#include <types.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "matrix/debug.h"
#include "matrix/const.h"
#include "hal/hal.h"
#include "hal/cpu.h"
#include "hal/spinlock.h"
#include "mm/malloc.h"
#include "mm/mmu.h"
#include "sys/time.h"
#include "timer.h"
#include "proc/process.h"
#include "proc/sched.h"

/* Number of priority levels */
#define NR_PRIORITIES	32

/* Time quantum to give to threads */
#define THREAD_QUANTUM	32

/* Run queue structure */
struct sched_queue {
	uint32_t bitmap;			// Bitmap of queues with data
	struct list threads[NR_PRIORITIES];	// Queues of runnable threads
};
typedef struct sched_queue sched_queue_t;

/* Per-CPU scheduling information structure */
struct sched_cpu {
	struct spinlock lock;			// Lock to protect information/queues
	
	struct thread *prev_thread;		// Previously executed thread
	struct thread *idle_thread;		// Thread scheduled when no other threads runnable

	struct timer timer;			// Preemption timer
	struct sched_queue *active;		// Active queue
	struct sched_queue *expired;		// Expired queue
	struct sched_queue queues[2];		// Active and expired queues
	
	size_t total;				// Total running/ready thread count
};
typedef struct sched_cpu sched_cpu_t;

/* Whether scheduler is ready */
boolean_t _scheduler_ready = FALSE;

/* Total number of running or ready threads across all CPUs */
static int _nr_running_threads = 0;

/* Dead process queue */
static struct list _dead_processes = {
	.prev = &_dead_processes,
	.next = &_dead_processes
};

/* Allocate a CPU for a thread to run on */
static struct cpu *sched_alloc_cpu(struct thread *t)
{
	size_t load, average, total;
	struct cpu *cpu, *other;
	struct list *l;

	/* On UP systems, the only choice is current CPU */
	if (_nr_cpus == 1) {
		return CURR_CPU;
	}

	/* Add 1 to the total number of threads to account for the thread we
	 * are adding.
	 */
	total = _nr_running_threads + 1;
	average = total / _nr_cpus;

	LIST_FOR_EACH(l, &_running_cpus) {
		other = LIST_ENTRY(l, struct cpu, link);
		load = other->sched->total;
		if (load < average) {
			cpu = other;
			break;
		}
	}
	
	return cpu;
}

/**
 * Add `p' to one of the queues of runnable processes. This function is responsible
 * for inserting a process into one of the scheduling queues. `p' must not in the
 * scheduling queues before enqueue.
 */
static void sched_enqueue(struct sched_queue *queue, struct thread *t)
{
	int q;
#ifdef _DEBUG_SCHED
	struct thread *thrd;
	struct list *l;
#endif	/* _DEBUG_SCHED */

	/* Determine where to insert the process */
	q = t->priority;

#ifdef _DEBUG_SCHED
	LIST_FOR_EACH(l, &queue->threads[q]) {
		thrd = LIST_ENTRY(l, struct thread, runq_link);
		if (thrd == t) {
			ASSERT(FALSE);
		}
	}
#endif	/* _DEBUG_SCHED */
	
	/* Now add the process to the tail of the queue. */
	ASSERT((q < NR_PRIORITIES) && (q >= 0));
	list_add_tail(&t->runq_link, &queue->threads[q]);
}

/**
 * A process must be removed from the scheduling queues, for example, because it has
 * been blocked.
 */
static void sched_dequeue(struct sched_queue *queue, struct thread *t)
{
	int q;
#ifdef _DEBUG_SCHED
	size_t found_times = 0;
	struct thread *thrd;
	struct list *l;
#endif	/* _DEBUG_SCHED */

	q = t->priority;

	/* Now make sure that the process is not in its ready queue. Remove the process
	 * if it was found.
	 */
	ASSERT((q < NR_PRIORITIES) && (q >= 0));
	list_del(&t->runq_link);
	
#ifdef _DEBUG_SCHED
	/* You can also just remove the specified process, but I just make sure it is
	 * really in our ready queue.
	 */
	LIST_FOR_EACH(l, &queue->threads[q]) {
		thrd = LIST_ENTRY(l, struct thread, runq_link);
		if (thrd == t) {
			found_times++;
		}
	}

	ASSERT(found_times == 0);		// The process should not be found
#endif	/* _DEBUG_SCHED */
}

static void sched_adjust_priority(struct sched_cpu *c, struct thread *t)
{
	;
}

/**
 * Pick a new process from the queue to run
 */
static struct thread *sched_pick_thread(struct sched_cpu *c)
{
	struct thread *t;
	struct list *l;
	int q;

	t = NULL;
	/* Check each of the scheduling queues for ready processes. The number of
	 * queues is defined in process.h. The lowest queue contains IDLE, which
	 * is always ready.
	 */
	for (q = NR_PRIORITIES - 1; q >= 0; q--) {
		if (!LIST_EMPTY(&c->active->threads[q])) {
			l = c->active->threads[q].next;
			t = LIST_ENTRY(l, struct thread, runq_link);
			sched_dequeue(c->active, t);
			break;
		}
	}

	return t;
}

void sched_insert_thread(struct thread *t)
{
	sched_cpu_t *sched;

	t->cpu = sched_alloc_cpu(t);
	
	sched = t->cpu->sched;
	
	sched_enqueue(sched->active, t);
	sched->total++;
}

/**
 * Picks a new process to run and switches to it. Interrupts must be disable.
 * @param state		- Previous interrupt state
 */
void sched_reschedule(boolean_t state)
{
	struct sched_cpu *c;
	struct thread *next;

	/* Check the interrupt state */
	ASSERT(irq_disable() == FALSE);

	/* Get current schedule CPU */
	c = CURR_CPU->sched;

	/* Adjust the priority of the thread based on whether it used up its quantum */
	if (CURR_THREAD != c->idle_thread) {
		sched_adjust_priority(c, CURR_THREAD);
	}

	/* Enqueue and dequeue the current process to update the process queue */
	if (CURR_THREAD->state == THREAD_RUNNING) {
		sched_enqueue(c->active, CURR_THREAD);
	} else {
		DEBUG(DL_DBG, ("sched_reschedule: p(%p), id(%d), state(%d).\n",
			       CURR_THREAD, CURR_THREAD->id, CURR_THREAD->state));
		c->total--;
	}
	
	/* Find a new process to run. A NULL return value means no processes are
	 * ready, so we schedule the idle process in this case.
	 */
	next = sched_pick_thread(c);
	if (next) {
		next->quantum = P_QUANTUM;
	} else {
		next = c->idle_thread;
		next->quantum = 0;
	}

	/* Move the next process to running state and set it as the current */
	c->prev_thread = CURR_THREAD;
	next->state = THREAD_RUNNING;
	CURR_THREAD = next;

	/* Perform the thread switch if current thread is not the same as previous
	 * one.
	 */
	if (CURR_THREAD != c->prev_thread) {
		DEBUG(DL_DBG, ("sched_reschedule: switching to (%s:%d:%d:%d).\n",
			       CURR_PROC->name, CURR_PROC->id, CURR_THREAD->id, CURR_CPU->id));
		
		/* Switch the address space. */
		mmu_switch_ctx(CURR_PROC->mmu_ctx);

		/* Perform the thread switch */
		arch_thread_switch(CURR_THREAD, c->prev_thread);

		/* Restore the IRQ */
		irq_restore(state);
	} else {
		irq_restore(state);
	}
}

static void sched_reaper_thread(void *ctx)
{
	/* If this is the first time reaper run, you should enable IRQ first */
	while (TRUE) {
		/* Reaper the dead threads */
		if (!LIST_EMPTY(&_dead_processes)) {
			struct list *p, *l;
			struct thread *t;
			
			LIST_FOR_EACH_SAFE(l, p, &_dead_processes) {
				t = LIST_ENTRY(l, struct thread, runq_link);
				list_del(&t->runq_link);
				DEBUG(DL_INF, ("sched_reaper_thread: release thread(%d).\n",
					       t->id));
				thread_release(t);
			}
		}
		
		sched_reschedule(FALSE);
	}
}

static void sched_idle_thread(void *ctx)
{
	/* We run the loop with interrupts disabled. The cpu_idle() function
	 * is expected to re-enable interrupts as required.
	 */
	irq_disable();

	while (TRUE) {
		sched_reschedule(FALSE);
		cpu_idle();
	}
}

void init_sched_percpu()
{
	int i, j, rc = -1;

	/* Initialize the scheduler for the current CPU */
	CURR_CPU->sched = kmalloc(sizeof(struct sched_cpu), 0);
	ASSERT(CURR_CPU->sched != NULL);
	
	CURR_CPU->sched->total = 0;
	CURR_CPU->sched->active = &CURR_CPU->sched->queues[0];
	CURR_CPU->sched->expired = &CURR_CPU->sched->queues[1];

	/* Create the per CPU idle thread */
	rc = thread_create(NULL, 0, sched_idle_thread, NULL,
			   &CURR_CPU->sched->idle_thread);
	ASSERT((rc == 0) && (CURR_CPU->sched->idle_thread != NULL));
	DEBUG(DL_DBG, ("init_sched_percpu: idle thread(%p).\n",
		       CURR_CPU->sched->idle_thread));

	/* Set the idle thread as the current thread */
	CURR_CPU->sched->idle_thread->cpu = CURR_CPU;
	CURR_CPU->sched->idle_thread->state = THREAD_RUNNING;
	CURR_CPU->sched->prev_thread = NULL;
	CURR_CPU->thread = CURR_CPU->sched->idle_thread;
	
	/* Create the preemption timer */
	init_timer(&CURR_CPU->sched->timer);

	/* Initialize queues */
	for (i = 0; i < 2; i++) {
		CURR_CPU->sched->queues[i].bitmap = 0;
		for (j = 0; j < NR_PRIORITIES; j++) {
			LIST_INIT(&CURR_CPU->sched->queues[i].threads[j]);
		}
	}
}

void init_sched()
{
	int rc = -1;

	/* Create kernel mode reaper thread for the whole system */
	rc = thread_create(NULL, 0, sched_reaper_thread, NULL, NULL);
	ASSERT(rc == 0);

	DEBUG(DL_DBG, ("init_sched: sched queues initialization done.\n"));
}

void sched_enter()
{
	/* Disable irq first as sched_insert_proc and process_switch requires */
	irq_disable();

	/* Set scheduler to ready */
	_scheduler_ready = TRUE;
	
	/* Switch to current process */
	arch_thread_switch(CURR_THREAD, NULL);
	PANIC("Should not get here");
}
