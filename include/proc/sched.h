#ifndef __SCHED_H__
#define __SCHED_H__

void sched_enqueue(struct task *tp);
void sched_dequeue(struct task *tp);

#endif	/* __SCHED_H__ */
