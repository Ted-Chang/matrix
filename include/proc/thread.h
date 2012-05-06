#ifndef __THREAD_H__
#define __THREAD_H__

#include "cpu.h"

typedef int thread_id_t;

struct process;

/* Entry function type for a thread */
typedef void (*thread_func_t)(void *);

/* Architecture-specific thread structure */
struct arch_thread {
	struct cpu *cpu;		// Current CPU pointer, used by curr_cpu
	uint32_t flags;			// Flags for the thread

	/* System call data */
	uint32_t kernel_stack;		// ESP for kernel entry via system call
	uint32_t user_stack;		// Temporary storage for user ESP

	/* Saved context switch stack pointer */
	uint32_t saved_stack;
};

/* Definition of thread */
struct thread {
	struct arch_thread arch;	// Architecture thread implementation
	
	enum {
		THREAD_CREATED,		// Newly created, not yet made runnable
		THREAD_READY,		// Ready and waiting to be run
		THREAD_RUNNING,		// Running on some CPU
		THREAD_SLEEPING,	// Sleeping, waiting for some event
		THREAD_DEAD,		// Dead, waiting to be cleaned up
	} state;

	struct spinlock lock;

	/* Main thread information */
	void *kstack;			// Kernel stack pointer
	uint32_t flags;
	int priority;			// Priority of the thread

	/* Scheduling information */
	struct list runq_link;		// Link to schedule queues
	int max_priority;		// Maximum scheduling priority
	int curr_priority;		// Current scheduling priority
	struct cpu *cpu;		// CPU the thread runs on
	useconds_t timeslice;		// Current timeslice

	/* Thread entry function */
	thread_func_t func;		// Entry function for the thread
	void *arg;			// Arguments to thread entry function

	/* Other thread information */
	uint32_t ustack;		// User-mode stack base
	size_t ustack_size;		// Size of the user-mode stack
	thread_id_t id;			// ID of the thread
	int status;			// Exit status of the thread

	struct process *owner;		// Pointer to parent process
	struct list owner_link;		// Link to parent process
};

/* Values for thread priority */
#define THREAD_PRIORITY_LOW	0
#define THREAD_PRIORITY_NORMAL	1
#define THREAD_PRIORITY_HIGH	2

/* Macro that expands to a pointer to the current thread */
#define CURR_THREAD	(CURR_CPU->thread)

extern status_t create_thread(struct process *owner, uint32_t flags,
			      thread_func_t func, void *arg,
			      struct thread **threadp);
extern void init_thread();

#endif	/* __THREAD_H__ */
