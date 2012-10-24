#ifndef __SCHED_H__
#define __SCHED_H__

extern void sched_insert_proc(struct process *proc);
extern void sched_remove_proc(struct process *proc);
extern void sched_reschedule(boolean_t state);
extern void init_sched_percpu();
extern void init_sched();

#endif	/* __SCHED_H__ */
