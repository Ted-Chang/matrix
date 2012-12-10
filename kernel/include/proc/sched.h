#ifndef __SCHED_H__
#define __SCHED_H__

extern void sched_insert_thread(struct thread *t);
extern void sched_post_switch(boolean_t state);
extern void sched_reschedule(boolean_t state);
extern void sched_enter();
extern void init_sched_percpu();
extern void init_sched();

#endif	/* __SCHED_H__ */
