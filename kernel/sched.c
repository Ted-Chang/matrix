#include <types.h>
#include <stddef.h>
#include <string.h>
#include "hal.h"
#include "lirq.h"
#include "proc/task.h"
#include "proc/sched.h"
#include "matrix/debug.h"

/* The schedule queue for our kernel */
struct task *_ready_head[NR_SCHED_QUEUES];
struct task *_ready_tail[NR_SCHED_QUEUES];

/* Pointer to our various task */
struct task *_next_task = NULL;
struct task *_prev_task = NULL;
struct task *_curr_task = NULL;				// Current running task

static void sched(struct task *tp, int *queue, boolean_t *front);

/**
 * pick_task only update the _next_task pointer.
 */
static void pick_task()
{
	/* Decide who to run now. A new task is selected by setting `_next_task'.
	 */
	struct task *tp;
	int q;

	/* Check each of the scheduling queues for ready tasks. The number of
	 * queues is defined in task.h. The lowest queue contains IDLE, which
	 * is always ready.
	 */
	for (q = 0; q < NR_SCHED_QUEUES; q++) {
		if ((tp = _ready_head[q]) != NULL) {
			_next_task = tp;
			break;
		}
	}

	DEBUG(DL_DBG, ("task %d picked.\n", _next_task->id));
}

void sched_init()
{
	memset(&_ready_head[0], 0, sizeof(struct task *) * NR_SCHED_QUEUES);
	memset(&_ready_tail[0], 0, sizeof(struct task *) * NR_SCHED_QUEUES);
}

/**
 * Add `tp' to one of the queues of runnable tasks. This function is responsible
 * for inserting a task into one of the scheduling queues. `tp' must not in the
 * scheduling queues before enqueue.
 */
void sched_enqueue(struct task *tp)
{
	int q;
	boolean_t front;

	irq_disable();

#ifdef _DEBUG_SCHED
	check_runqueues("sched_enqueue:begin");
#endif
	
	/* Determine where to insert the task */
	sched(tp, &q, &front);
	
	/* Now add the task to the queue. */
	if (_ready_head[q] == NULL) {			// Add to empty queue
		_ready_head[q] = _ready_tail[q] = tp;
		tp->next = NULL;
	} else if (front) {				// Add to head of queue
		tp->next = _ready_head[q];
		_ready_head[q] = tp;
	} else {					// Add to tail of queue
		_ready_tail[q]->next = tp;
		_ready_tail[q] = tp;
		tp->next = NULL;
	}

	/* Now select the next task to run */
	pick_task();
	
#ifdef _DEBUG_SCHED
	check_runqueues("sched_enqueue:end");
#endif

	irq_enable();
}

/**
 * A task must be removed from the scheduling queues, for example, because it has
 * been blocked. If current active task is removed, a new task is picked by calling
 * pick_task()
 */
void sched_dequeue(struct task *tp)
{
	int q;
	struct task **xpp;
	struct task *prev_ptr;

	irq_disable();
	
	q = tp->priority;

#ifdef _DEBUG_SCHED
	check_runqueues("sched_dequeue:begin");
#endif
	
	/* Now make sure that the task is not in its ready queue. Remove the task
	 * if it was found.
	 */
	prev_ptr = NULL;
	for (xpp = &_ready_head[q]; *xpp != NULL; xpp = &((*xpp)->next)) {
		
		if (*xpp == tp) {			// Found task to remove
			*xpp = (*xpp)->next;		// Replace with the next in chain
			if (tp == _ready_tail[q])	// Queue tail removed
				_ready_tail[q] = prev_ptr; // Set new tail
			if ((tp == _curr_task) || (tp == _next_task)) // Active task removed
				pick_task();		// Pick a new task to run
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
 * This function determines the scheduling policy. It is called whenever a task
 * must be added to one of the scheduling queues to decide where to insert it.
 * As a side-effect the process' priority may be updated. This function will
 * update the _prev_task pointer.
 */
static void sched(struct task *tp, int *queue, boolean_t *front)
{
	boolean_t time_left = (tp->ticks_left > 0);	// Quantum fully consumed
	int penalty = 0;				// Change in priority

	/* Check whether the task has time left. Otherwise give a new quantum
	 * and possibly raise the priority.
	 */
	if (!time_left) {				// Quantum consumed ?
		tp->ticks_left = tp->quantum;		// Give new quantum
		//if (_prev_task == tp)
		//	penalty++;			// Priority boost
		//else
		//	penalty--;			// Give slow way back
		_prev_task = tp;			// Store ptr for next
	}

	/* Determine the new priority of this task. The bounds are determined
	 * by IDLE's queue and the maximum priority of this task.
	 */
	if (penalty != 0) {
		tp->priority += penalty;		// Update with penalty
		if (tp->priority < tp->max_priority)	// Check upper bound
			tp->priority = tp->max_priority;
		else if (tp->priority > (IDLE_Q + 1))	// Check lower bound
			tp->priority = IDLE_Q;
	}

	/* If there is time left, the task is added to the front of its queue,
	 * so that it can run immediately. The queue to use is simply the current
	 * task's priority.
	 */
	*queue = tp->priority;
	*front = time_left;
}
