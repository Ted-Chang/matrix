#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/debug.h"
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
#define THREAD_QUANTUM

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

/* The schedule queue for our kernel */
struct process *_ready_head[NR_SCHED_QUEUES];
struct process *_ready_tail[NR_SCHED_QUEUES];

/* Pointer to our various task */
volatile struct process *_next_proc = NULL;
volatile struct process *_prev_proc = NULL;
volatile struct process *_curr_proc = NULL;		// Current running process

boolean_t _sched_initialized = FALSE;

static void sched(struct process *tp, int *queue, boolean_t *front);

static struct cpu *sched_alloc_cpu(struct thread *t)
{
	struct cpu *cpu, *other;

	/* On UP systems, the only choice is current CPU */
	if (_nr_cpus == 1) {
		return CURR_CPU;
	}

	cpu = NULL;
	
	return cpu;
}

/**
 * pick_process only update the _next_proc pointer.
 */
static void pick_process()
{
	/* Decide who to run now. A new process is selected by setting `_next_proc'.
	 */
	struct process *p;
	int q;

	/* Check each of the scheduling queues for ready processes. The number of
	 * queues is defined in process.h. The lowest queue contains IDLE, which
	 * is always ready.
	 */
	for (q = 0; q < NR_SCHED_QUEUES; q++) {
		if ((p = _ready_head[q]) != NULL) {
			_next_proc = p;
			break;
		}
	}
}

/**
 * Add `p' to one of the queues of runnable processes. This function is responsible
 * for inserting a process into one of the scheduling queues. `p' must not in the
 * scheduling queues before enqueue.
 */
void sched_enqueue(struct process *p)
{
	int q;
	boolean_t front;

	irq_disable();

#ifdef _DEBUG_SCHED
	check_runqueues("sched_enqueue:begin");
#endif
	
	/* Determine where to insert the process */
	sched(p, &q, &front);
	
	/* Now add the process to the queue. */
	if (_ready_head[q] == NULL) {			// Add to empty queue
		_ready_head[q] = _ready_tail[q] = p;
		p->next = NULL;
	} else if (front) {				// Add to head of queue
		p->next = _ready_head[q];
		_ready_head[q] = p;
	} else {					// Add to tail of queue
		_ready_tail[q]->next = p;
		_ready_tail[q] = p;
		p->next = NULL;
	}

	/* Now select the next process to run */
	pick_process();
	
#ifdef _DEBUG_SCHED
	check_runqueues("sched_enqueue:end");
#endif

	irq_enable();
}

/**
 * A process must be removed from the scheduling queues, for example, because it has
 * been blocked. If current active process is removed, a new process is picked by calling
 * pick_process()
 */
void sched_dequeue(struct process *p)
{
	int q;
	struct process **xpp;
	struct process *prev_ptr;

	irq_disable();
	
	q = p->priority;

#ifdef _DEBUG_SCHED
	check_runqueues("sched_dequeue:begin");
#endif
	
	/* Now make sure that the process is not in its ready queue. Remove the process
	 * if it was found.
	 */
	prev_ptr = NULL;
	for (xpp = &_ready_head[q]; *xpp != NULL; xpp = &((*xpp)->next)) {
		
		if (*xpp == p) {			// Found process to remove
			*xpp = (*xpp)->next;		// Replace with the next in chain
			if (p == _ready_tail[q])	// Queue tail removed
				_ready_tail[q] = prev_ptr; // Set new tail
			if ((p == _curr_proc) ||
			    (p == _next_proc))		// Active process removed
				pick_process();		// Pick a new process to run
			break;				// Break out the for loop
		}
		prev_ptr = *xpp;
	}

#ifdef _DEBUG_SCHED
	check_runqueues("sched_dequeue:end");
#endif

	irq_disable();
}

/**
 * This function determines the scheduling policy. It is called whenever a process
 * must be added to one of the scheduling queues to decide where to insert it.
 * As a side-effect the process' priority may be updated. This function will
 * update the _prev_proc pointer.
 */
static void sched(struct process *p, int *queue, boolean_t *front)
{
	boolean_t time_left = (p->ticks_left > 0);	// Quantum fully consumed
	int penalty = 0;				// Change in priority

	/* Check whether the process has time left. Otherwise give a new quantum
	 * and possibly raise the priority.
	 */
	if (!time_left) {				// Quantum consumed ?
		p->ticks_left = p->quantum;		// Give new quantum
		_prev_proc = p;				// Store ptr for next
	}

	/* Determine the new priority of this process. The bounds are determined
	 * by IDLE's queue and the maximum priority of this process.
	 */
	if (penalty != 0) {
		p->priority += penalty;			// Update with penalty
		if (p->priority < p->max_priority)	// Check upper bound
			p->priority = p->max_priority;
		else if (p->priority > (IDLE_Q + 1))	// Check lower bound
			p->priority = IDLE_Q;
	}

	/* If there is time left, the process is added to the front of its queue,
	 * so that it can run immediately. The queue to use is simply the current
	 * process's priority.
	 */
	*queue = p->priority;
	*front = time_left;
}

void init_sched_percpu()
{
	int i, j;

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
	if (_sched_initialized) return;
	
	_sched_initialized = TRUE;
	memset(&_ready_head[0], 0, sizeof(struct process *) * NR_SCHED_QUEUES);
	memset(&_ready_tail[0], 0, sizeof(struct process *) * NR_SCHED_QUEUES);
}
