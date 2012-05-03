#ifndef __THREAD_H__
#define __THREAD_H__

#include "cpu.h"

/* Definition of thread */
struct thread {
	enum {
		THREAD_CREATED,
		THREAD_READY,
		THREAD_RUNNING,
		THREAD_SLEEPING,
		THREAD_DEAD,
	} state;

	struct spinlock lock;

	void *kstack;
	uint32_t flags;
	int priority;
	struct cpu *cpu;

	//char name[MAX_THREAD_NAME];
	int status;
};

/* Macro that expands to a pointer to the current thread */
#define CURR_THREAD	(CURR_CPU->thread)

extern void init_thread();

#endif	/* __THREAD_H__ */
