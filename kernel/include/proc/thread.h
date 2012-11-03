#ifndef __THREAD_H__
#define __THREAD_H__

#include <types.h>
#include "list.h"
#include "hal/cpu.h"
#include "hal/spinlock.h"
#include "avltree.h"
#include "notifier.h"

struct process;

/* Thread entry definition */
typedef void (*thread_func_t)(void *);

/* x86-specific thread structure */
struct arch_thread {
	void *esp;			// Stack pointer
	void *ebp;			// Base pointer
	void *eip;			// Instruction pointer

	struct registers *syscall_regs;	// Store the regs on stack for this thread
};

/* Definition of a thread */
struct thread {
	/* Architecture thread implementation */
	struct arch_thread arch;

	/* State of the thread */
	enum {
		THREAD_CREATED,
		THREAD_READY,
		THREAD_RUNNING,
		THREAD_SLEEPING,
		THREAD_DEAD,
	} state;

	/* Lock for the thread */
	struct spinlock lock;

	/* Main thread information */
	void *kstack;			// Kernel stack pointer
	void *ustack;			// User-mode stack base
	size_t ustack_size;		// Size of the user-mode stack
	int flags;			// Flags for the thread
	int priority;			// Priority of the thread

	/* Thread entry function */
	thread_func_t entry;		// Entry function for the thread
	void *args;			// Argument to thread entry function

	/* Scheduling information */
	struct list runq_link;		// Link to run queues
	int max_priority;		// Maximum scheduling priority
	int curr_priority;		// Current scheduling priority
	struct cpu *cpu;		// CPU that the thread runs on
	useconds_t quantum;		// Current quantum

	/* Sleeping information */
	struct list wait_link;		// Link to a waiting list
	int sleep_status;

	/* Reference count for the thread */
	int ref_count;

	/* Other thread information */
	tid_t id;			// ID of the thread
	struct avl_tree_node tree_link;	// Link to thread tree
	int status;			// Exit status of the thread
	struct process *owner;		// Pointer to parent process
	struct list owner_link;		// Link to the owner process

	struct notifier death_notifier;	// Notifier list of this thread
};
typedef struct thread thread_t;

/* Macro that expands to a pointer to the current thread */
#define CURR_THREAD	(CURR_CPU->thread)

extern void arch_thread_switch(struct thread *curr, struct thread *prev);
extern int thread_create(struct process *owner, int flags, thread_func_t func,
			 void *args, struct thread **tp);
extern void thread_run(struct thread *t);
extern void thread_kill(struct thread *t);
extern void thread_release(struct thread *t);
extern void thread_exit();
extern void init_thread();

#endif	/* __THREAD_H__ */
