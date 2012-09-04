#ifndef __SCHED_H__
#define __SCHED_H__

void sched_enqueue(struct process *p);
void sched_dequeue(struct process *p);

#endif	/* __SCHED_H__ */
