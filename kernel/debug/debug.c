#include <types.h>
#include <stddef.h>
#include "hal/hal.h"
#include "matrix/debug.h"
#include "proc/process.h"

uint32_t _debug_level = DL_INF;

void panic(const char *message, const char *file, uint32_t line)
{
	irq_disable();	// Disable all interrupts
	kprintf("PANIC(%s) at %s:%d\n", message, file, line);
	for (; ; ) ;
}

void panic_assert(const char *file, uint32_t line, const char *desc)
{
	irq_disable();
	kprintf("ASSERTION FAILED(%s) at %s:%d\n", desc, file, line);
	for (; ; ) ;
}

#ifdef _DEBUG_SCHED

void check_runqueues(char *when)
{
	int q;
	struct process *p;

	//for (q = 0; q < NR_PRIORITIES; q++) {
		/* if (_ready_head[q] && !_ready_tail[q]) { */
		/* 	kprintf("head but no tail, priority(%d): %s\n", q, when); */
		/* 	PANIC("Scheduling error!"); */
		/* } */

		/* if (!_ready_head[q] && _ready_tail[q]) { */
		/* 	kprintf("tail but no head, priority(%d): %s\n", q, when); */
		/* 	PANIC("Scheduling error!"); */
		/* } */

		/* if (_ready_tail[q] && _ready_tail[q]->next != NULL) { */
		/* 	kprintf("tail and tail->next not null, priority(%d): %s\n", */
		/* 		q, when); */
		/* 	PANIC("Scheduling error!"); */
		/* } */

		/* for (p = _ready_head[q]; p != NULL; p = p->next) { */
		/* 	if ((p->next == NULL) && (_ready_tail[q] != p)) { */
		/* 		kprintf("last element not tail: %s\n", when); */
		/* 		PANIC("Scheduling error!"); */
		/* 	} */
		/* } */
	//}
}

#endif	/* _DEBUG_SCHED */
