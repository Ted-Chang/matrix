#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/debug.h"
#include "matrix/const.h"
#include "hal/hal.h"
#include "hal/cpu.h"
#include "hal/spinlock.h"
#include "mm/malloc.h"
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

/* Total number of running or ready threads across all CPUs */
static int _nr_running_threads = 0;

/* The schedule queue for our kernel */
struct list _ready_queue[NR_PRIORITIES];
struct list _expired_queue[NR_PRIORITIES];

/* Pointer to our various task */
struct process *_prev_proc = NULL;
struct process *_curr_proc = NULL;		// Current running process

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
static void sched_enqueue(struct list *queue, struct process *p)
{
	int q;
#ifdef _DEBUG_SCHED
	boolean_t proc_found = FALSE;
	struct process *proc;
	struct list *l, *temp;
#endif	/* _DEBUG_SCHED */

	/* Determine where to insert the process */
	q = p->priority;
	
	/* Now add the process to the tail of the queue. */
	ASSERT(q < NR_PRIORITIES);
	if (p->state == PROCESS_RUNNING) {
		list_add_tail(&p->link, &_ready_queue[q]);
	} else if (p->state == PROCESS_WAIT) {
		DEBUG(DL_DBG, ("sched_enqueue: added to expire queue, pid(%d).\n", p->id));
		list_add_tail(&p->link, &_expired_queue[q]);
	}

#ifdef _DEBUG_SCHED
	if (p->state == PROCESS_RUNNING) {
		temp = _ready_queue;
	} else if (p->state == PROCESS_WAIT) {
		temp = _expired_queue;
	} else {
		ASSERT(FALSE);
	}
	
	LIST_FOR_EACH(l, &temp[q]) {
		proc = LIST_ENTRY(l, struct process, link);
		if (proc == p) {
			proc_found = TRUE;
			break;
		}
	}

	ASSERT(proc_found == TRUE);
#endif	/* _DEBUG_SCHED */
}

/**
 * A process must be removed from the scheduling queues, for example, because it has
 * been blocked.
 */
static void sched_dequeue(struct list *queue, struct process *p)
{
	int q;
#ifdef _DEBUG_SCHED
	boolean_t proc_found = FALSE;
	struct process *proc;
	struct list *l, *temp;
#endif	/* _DEBUG_SCHED */

	q = p->priority;

	/* Now make sure that the process is not in its ready queue. Remove the process
	 * if it was found.
	 */
	ASSERT(q < NR_PRIORITIES);
	list_del(&p->link);
	
#ifdef _DEBUG_SCHED
	/* You can also just remove the specified process, but I just make sure it is
	 * really in our ready queue.
	 */
	if (p->state == PROCESS_RUNNING) {
		temp = _ready_queue;
	} else if (p->state == PROCESS_WAIT) {
		temp = _expired_queue;
	} else {
		ASSERT(FALSE);
	}
	
	LIST_FOR_EACH(l, &temp[q]) {
		proc = LIST_ENTRY(l, struct process, link);
		if (proc == p) {
			proc_found = TRUE;
			break;
		}
	}

	ASSERT(proc_found == FALSE);		// The process should not be found
#endif	/* _DEBUG_SCHED */
}

static void sched_adjust_priority(struct sched_cpu *c, struct process *p)
{
	;
}

/**
 * Pick a new process from the queue to run
 */
static struct process *sched_pick_process(struct sched_cpu *c)
{
	struct process *p;
	struct list *l;
	int q;

	/* Check each of the scheduling queues for ready processes. The number of
	 * queues is defined in process.h. The lowest queue contains IDLE, which
	 * is always ready.
	 */
	for (q = 0; q < NR_PRIORITIES; q++) {
		if (!LIST_EMPTY(&_ready_queue[q])) {
			l = _ready_queue[q].next;
			p = LIST_ENTRY(l, struct process, link);
			ASSERT(p != NULL);
			break;
		}
	}

	return p;
}

void sched_insert_proc(struct process *proc)
{
	sched_enqueue(NULL, proc);	// FixMe: Choose which queue to insert
}

void sched_remove_proc(struct process *proc)
{
	sched_dequeue(NULL, proc);	// FixMe: This code will be replaced in the future
}

/**
 * Picks a new process to run and switches to it. Interrupts must be disable.
 * @param state		- Previous interrupt state
 */
void sched_reschedule(boolean_t state)
{
	struct sched_cpu *c;
	struct process *next;

	c = CURR_CPU->sched;

	/* Adjust the priority of the thread based on whether it used up its quantum */
	sched_adjust_priority(c, CURR_PROC);

	/* Enqueue and dequeue the current process to update the process queue */
	sched_dequeue(NULL, CURR_PROC);
	sched_enqueue(NULL, CURR_PROC);
	
	/* Find a new process to run. A NULL return value means no processes are
	 * ready, so we schedule the idle process in this case.
	 */
	next = sched_pick_process(c);
	ASSERT(next != NULL);
	next->quantum = P_QUANTUM;

	/* Move the next process to running state and set it as the current */
	_prev_proc = CURR_PROC;
	CURR_PROC = next;

	/* Perform the process switch if current process is not the same as previous
	 * one.
	 */
	if (CURR_PROC != _prev_proc) {
		process_switch(CURR_PROC, _prev_proc);
		irq_restore(state);
	} else {
		irq_restore(state);
	}
}

void init_sched_percpu()
{
	int i, j;

	/* Initialize the scheduler for the current CPU */
	CURR_CPU->sched = kmalloc(sizeof(struct sched_cpu), 0);
	CURR_CPU->sched->total = 0;
	CURR_CPU->sched->active = &CURR_CPU->sched->queues[0];
	CURR_CPU->sched->expired = &CURR_CPU->sched->queues[1];

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
	int i;

	/* Initialize the priority queues for process */
	for (i = 0; i < NR_PRIORITIES; i++) {
		LIST_INIT(&_ready_queue[i]);
		LIST_INIT(&_expired_queue[i]);
	}

	/* Disable irq first as sched_insert_proc and process_switch requires */
	irq_disable();

	/* Enqueue the kernel process which is the first process in our system */
	sched_insert_proc(_kernel_proc);
	
	DEBUG(DL_DBG, ("init_sched: kernel process(%p).\n", _kernel_proc));

	/* At this time we can schedule the process now */
	CURR_PROC = _kernel_proc;

	/* Switch to current process, we set previous process to CURR_PROC so we
	 * can get the context initialized.
	 */
	process_switch(CURR_PROC, CURR_PROC);

	/* Restore the irq */
	irq_restore(TRUE);
}
