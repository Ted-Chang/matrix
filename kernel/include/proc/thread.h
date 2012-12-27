#ifndef __THREAD_H__
#define __THREAD_H__

#include "list.h"
#include "matrix/const.h"
#include "hal/cpu.h"
#include "hal/spinlock.h"
#include "rtl/avltree.h"
#include "rtl/notifier.h"
#include "timer.h"
#include "proc/signal.h"

struct process;

/* Thread entry definition */
typedef void (*thread_func_t)(void *);

/* x86-specific thread structure */
struct arch_thread {
	void *esp;			// Stack pointer
	void *ebp;			// Base pointer
	void *eip;			// Instruction pointer
};

/* Thread creation arguments structure, for thread_uspace_wrapper() */
struct thread_uspace_creation {
	ptr_t entry;			// Instruction pointer
	ptr_t esp;			// Stack pointer
	ptr_t args;			// Argument
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
	struct cpu *cpu;		// CPU that the thread runs on
	useconds_t quantum;		// Current quantum

	/* Sleeping information */
	struct spinlock *wait_lock;	// Lock to acquire when perform waiting
	struct list wait_link;		// Link to a waiting list
	struct timer sleep_timer;	// Sleep timeout timer
	int sleep_status;		// Sleep status (timed out/interrupted)

	/* Reference count for the thread
	 * A running thread always has at least 1 reference on it.
	 */
	int ref_count;

	/* Signal information */
	sigset_t signal_mask;		// Signal mask for the thread
	sigset_t pending_signals;	// Bitmap of pending signals
	struct siginfo signal_info[NSIG]; // Information associated with pending signals
	struct stack signal_stack;	// Alternate signal stack

	/* Other thread information */
	tid_t id;			// ID of the thread
	char name[T_NAME_LEN];		// Name of the thread
	struct avl_tree_node tree_link;	// Link to thread tree
	struct process *owner;		// Pointer to parent process
	struct list owner_link;		// Link to the owner process

	struct notifier death_notifier;	// Notifier list of this thread

	int status;			// Exit status of the thread
};
typedef struct thread thread_t;

/* Internal flags for a thread */
#define THREAD_INTERRUPTIBLE_F	(1<<0)	// Thread is in an interruptible sleep
#define THREAD_INTERRUPTED_F	(1<<1)	// Thread has been interrupted
#define THREAD_KILLED_F		(1<<2)	// Thread has been killed

/* Macro that expands to a pointer to the current thread */
#define CURR_THREAD	(CURR_CPU->thread)

extern void arch_thread_switch(struct thread *curr, struct thread *prev);
extern void arch_thread_enter_uspace(ptr_t entry, ptr_t ustack, ptr_t ctx);
extern void thread_uspace_wrapper(void *ctx);
extern int thread_create(const char *name, struct process *owner, int flags,
			 thread_func_t func, void *args, struct thread **tp);
extern int thread_sleep(struct spinlock *lock, useconds_t timeout,
			const char *name, int flags);
extern void thread_run(struct thread *t);
extern void thread_kill(struct thread *t);
extern void thread_release(struct thread *t);
extern void thread_wake(struct thread *t);
extern boolean_t thread_interrupt(struct thread *t);
extern void thread_exit();
extern void init_thread();

#endif	/* __THREAD_H__ */
