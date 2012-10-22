#ifndef __SCHED_H__
#define __SCHED_H__

extern void sched_enqueue(struct process *p);
extern void sched_dequeue(struct process *p);
extern void sched_reschedule(boolean_t state);
extern void init_sched_percpu();
extern void init_sched();

#endif	/* __SCHED_H__ */
