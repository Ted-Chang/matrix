#include <types.h>
#include <stddef.h>
#include "hal.h"
#include "matrix/debug.h"
#include "matrix/global.h"
#include "proc/task.h"

uint32_t _debug_level = DL_DBG;

void panic(const char *message, const char *file, uint32_t line)
{
	disable_interrupt();	// Disable all interrupts
	kprintf("PANIC(%s) at %s:%d\n", message, file, line);
	for (; ; ) ;
}

void panic_assert(const char *file, uint32_t line, const char *desc)
{
	disable_interrupt();
	kprintf("ASSERTION FAILED(%s) at %s:%d\n", desc, file, line);
	for (; ; ) ;
}

#ifdef _DEBUG_SCHED

void check_runqueues(char *when)
{
	int q, l;
	struct task *tp;

	for (q = 0; q < NR_SCHED_QUEUES; q++) {
		if (_ready_head[q] && !_ready_tail[q]) {
			kprintf("head but no tail, priority(%d): %s\n", q, when);
			PANIC("Scheduling error!");
		}

		if (!_ready_head[q] && _ready_tail[q]) {
			kprintf("tail but no head, priority(%d): %s\n", q, when);
			PANIC("Scheduling error!");
		}

		if (_ready_tail[q] && _ready_tail[q]->next != NULL) {
			kprintf("tail and tail->next not null, priority(%d): %s\n",
				q, when);
			PANIC("Scheduling error!");
		}

		for (tp = _ready_head[q]; tp != NULL; tp = tp->next) {
			if (tp->priority != q) {
				kprintf("wrong priority: %s\n", when);
				PANIC("Scheduling error!");
			}
			if ((tp->next == NULL) && (_ready_tail[q] != tp)) {
				kprintf("last element not tail: %s\n", when);
				PANIC("Scheduling error!");
			}
		}
	}
}

#endif	/* _DEBUG_SCHED */
