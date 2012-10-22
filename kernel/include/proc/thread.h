#ifndef __THREAD_H__
#define __THREAD_H__

#include <types.h>
#include "list.h"
#include "hal/cpu.h"
#include "hal/spinlock.h"
#include "avltree.h"

struct process;

/* Thread entry definition */
typedef void (*thread_func_t)(void *);

/* x86-specific thread structure */
struct arch_thread {
	struct cpu *cpu;		// Current CPU pointer, used by CURR_CPU

	/* SYSCALL/SYSRET data */
	void *kernel_esp;		// ESP for kernel entry via SYSCALL
	void *user_esp;			// Temporary storage for user ESP

	/* Saved context switch stack pointer */
	void *saved_esp;

	int flags;			// Flags for the thread
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
	int flags;			// Flags for the thread
	int priority;			// Priority of the thread

	/* Scheduling information */
	struct list runq_link;		// Link to run queues
	int max_priority;		// Maximum scheduling priority
	int curr_priority;		// Current scheduling priority
	struct cpu *cpu;		// CPU that the thread runs on
	useconds_t quantum;		// Current quantum

	/* Reference count for the thread */
	int ref_count;

	/* Thread entry function */
	thread_func_t entry;		// Entry function for the thread
	void *args;			// Argument to thread entry function

	/* Other thread information */
	void *ustack;			// User-mode stack base
	size_t ustack_size;		// Size of the user-mode stack
	tid_t id;			// ID of the thread
	struct avl_tree_node tree_link;	// Link to thread tree
	int status;			// Exit status of the thread
	struct process *owner;		// Pointer to parent process
};
typedef struct thread thread_t;

/* Macro that expands to a pointer to the current thread */
#define CURR_THREAD	()

extern void init_thread();

#endif	/* __THREAD_H__ */
