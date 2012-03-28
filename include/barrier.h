#ifndef __BARRIER_H__
#define __BARRIER_H__

/* The barrier here is compiler specific */

/* Barrier for entering a critical section */
#define enter_cs_barrier()	asm volatile("" ::: "memory")

/* Barrier for leaving a critical section */
#define leave_cs_barrier()	asm volatile("" ::: "memory")

#endif	/* __BARRIER_H__ */
