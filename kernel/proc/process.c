/*
 * process.c
 */

#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/matrix.h"
#include "matrix/const.h"
#include "matrix/debug.h"
#include "hal/hal.h"
#include "hal/cpu.h"
#include "mm/page.h"
#include "mm/mmu.h"
#include "mm/malloc.h"
#include "proc/process.h"
#include "proc/sched.h"
#include "proc/thread.h"
#include "elf.h"
#include "object.h"
#include "semaphore.h"

struct process_creation {
	/* Arguments provided by the caller */
	char **argv;		// Arguments array
	char **env;		// Environment array
	int argc;		// Argument count

	/* Information used internally by the loader */
	struct mmu_ctx *mmu;	// Address space for the process

	/* Information to return to the caller */
	int status;		// Status code to return from the call
};

static pid_t _next_pid = 1;

/* Tree of all processes */
static struct avl_tree _proc_tree;

/* kernel process */
struct process *_kernel_proc = NULL;

extern uint32_t _initial_esp;

static pid_t id_alloc()
{
	return _next_pid++;
}

/* Move stack to a new position */
static void move_stack(uint32_t new_stack, uint32_t size)
{
	uint32_t i, pd_addr;
	uint32_t old_esp, old_ebp;
	uint32_t new_esp, new_ebp;
	uint32_t offset;

	/* Allocate some space for the new stack */
	for (i = new_stack; i >= (new_stack - size); i -= PAGE_SIZE) {
		/* General purpose stack is in user-mode */
		struct page *page;
		
		page = mmu_get_page(CURR_ASPACE, i, TRUE, 0);
		/* Associate the pte with a physical page */
		page_alloc(page, FALSE, TRUE);
	}

	/* Flush the TLB by reading and writing the page directory address again */
	asm volatile("mov %%cr3, %0" : "=r"(pd_addr));
	asm volatile("mov %0, %%cr3" :: "r"(pd_addr));

	/* Old ESP and EBP, read from registers */
	asm volatile("mov %%esp, %0" : "=r"(old_esp));
	asm volatile("mov %%ebp, %0" : "=r"(old_ebp));

	offset = (uint32_t)new_stack - _initial_esp;
	
	/* Initialize the new ESP and EBP */
	new_esp = old_esp + offset;
	new_ebp = old_ebp + offset;

	DEBUG(DL_DBG, ("move stack to new address.\n"
		       "- old_esp(0x%x), old_ebp(0x%x)\n"
		       "- new_esp(0x%x), new_ebp(0x%x)\n"
		       "- _initial_esp(0x%x)\n",
		       old_esp, old_ebp, new_esp, new_ebp, _initial_esp));
	
	/* Copy the old stack to new stack. Although we have switched to cloned
	 * page directory, the kernel memory map was not changed. So we can just
	 * copy the data between the initial esp and the old esp to our new stack.
	 */
	memcpy((void *)new_esp, (void *)old_esp, _initial_esp - old_esp);

	/* Backtrace through the original stack, copying new values into
	 * the new stack
	 */
	for (i = (uint32_t)new_stack; i > ((uint32_t)new_stack - size); i -= 4) {
		uint32_t tmp = *(uint32_t *)i;

		/* If the value of tmp is inside the range of the old stack, assume
		 * it is a base pointer and remap it. This will unfortunately remap
		 * ANY value in this range, whether they are base pointers or not.
		 */
		if ((old_esp < tmp) && (tmp < _initial_esp)) {
			uint32_t *tmp2;
			
			tmp = tmp + offset;
			tmp2 = (uint32_t *)i;
			*tmp2 = tmp;
		}
	}

	/* Change to the new stack */
	asm volatile("mov %0, %%esp" :: "r"(new_esp));
	asm volatile("mov %0, %%ebp" :: "r"(new_ebp));
}

/**
 * Process constructor for slab allocator
 */
static void process_ctor(void *obj)
{
	struct process *p;

	p = (struct process *)obj;

	/* Initialize the signature */
	p->ref_count = 0;

	LIST_INIT(&p->threads);

	/* Initialize the death notifier */
	init_notifier(&p->death_notifier);

	p->status = 0;
}

/**
 * Process destructor for slab allocator
 */
static void process_dtor(void *obj)
{
	DEBUG(DL_DBG, ("process (%p) destructed.\n", obj));
}

static void process_release(struct process *p)
{
	if (atomic_dec(&p->ref_count) == 0) {
		process_destroy(p);
	}
}

static void process_cleanup(struct process *p)
{
	if (p->mmu_ctx) {
		mmu_destroy_ctx(p->mmu_ctx);
		p->mmu_ctx = NULL;
	}
	
	if (p->fds) {
		fd_table_destroy(p->fds);
		p->fds = NULL;
	}
}

static int process_alloc(const char *name, struct process *parent, struct mmu_ctx *mmu,
			 int flags, int priority, struct fd_table *fds,
			 struct process **procp)
{
	int rc = -1;
	struct process *p;

	ASSERT((name != NULL) && (procp != NULL));

	/* Create the new process */
	p = kmalloc(sizeof(struct process), 0);
	if (!p) {
		DEBUG(DL_DBG, ("kmalloc process failed.\n"));
		rc = -1;
		goto out;
	}
	memset(p, 0, sizeof(struct process));

	/* Process constructor */
	process_ctor(p);

	/* Allocate a process ID. If creating the kernel process, always give
	 * it an ID of 0.
	 */
	if (_kernel_proc) {
		p->id = id_alloc();	// Allocate an ID for the process
	} else {
		p->id = 0;		// The first process must be kernel process
	}
	
	strncpy(p->name, name, P_NAME_LEN - 1);
	p->name[P_NAME_LEN - 1] = 0;

	p->uid = 500;			// FixMe: set it to the currently logged user
	p->gid = 500;			// FixMe: set it to the current user's group
	p->mmu_ctx = mmu;		// MMU context
	p->priority = priority;
	p->flags = flags;
	p->status = 0;
	
	/* Initialize the file descriptor table */
	if (!fds) {
		p->fds = fd_table_create();
		if (!p->fds) {
			rc = -1;
			goto out;
		}
	} else {
		p->fds = fds;
	}

	/* Insert this process into process tree */
	avl_tree_insert_node(&_proc_tree, &p->tree_link, p->id, p);

	p->state = PROCESS_RUNNING;
	*procp = p;
	rc = 0;

	DEBUG(DL_DBG, ("allocated process(%s:%d:%p:%p).\n",
		       p->name, p->id, p, p->mmu_ctx));
out:
	if (rc != 0) {
		if (p) {
			kfree(p);
		}
	}
	
	return rc;
}

/**
 * Lookup a process of the specified pid
 */
struct process *process_lookup(pid_t pid)
{
	struct process *proc;

	// TODO: Use a lock to protect this operation
	proc = avl_tree_lookup(&_proc_tree, pid);

	return proc;
}

/**
 * Attach a thread to a process
 */
void process_attach(struct process *p, struct thread *t)
{
	t->owner = p;
	ASSERT(p->state != PROCESS_DEAD);
	list_add_tail(&t->owner_link, &p->threads);
	atomic_inc(&p->ref_count);
}

/**
 * Detach a thread from its owner
 */
void process_detach(struct thread *t)
{
	struct process *p;

	p = t->owner;
	list_del(&t->owner_link);

	/* Move the process to the dead state if no threads is alive */
	if (LIST_EMPTY(&p->threads)) {
		ASSERT(p->state != PROCESS_DEAD);
		p->state = PROCESS_DEAD;
		process_cleanup(p);

		/* Run the death notifier */
		notifier_run(&p->death_notifier);
	}
	
	t->owner = NULL;
	process_release(p);
}

int process_exit(int status)
{
	struct thread *t;
	struct list *l;
	size_t n = 0;

	LIST_FOR_EACH(l, &CURR_PROC->threads) {
		t = LIST_ENTRY(l, struct thread, owner_link);
		if (t != CURR_THREAD) {
			thread_kill(t);
		}
		n++;
	}

	DEBUG(DL_DBG, ("process(%s:%d), n(%d).\n", CURR_PROC->name,
		       CURR_PROC->id, n));
	
	CURR_PROC->status = status;

	thread_exit();

	return 0;
}

static void process_entry_thread(void *ctx)
{
	int rc = -1;
	void *entry = NULL;
	struct process_creation *info;

	info = (struct process_creation *)ctx;

	ASSERT(CURR_ASPACE == info->mmu);

	/* Load the ELF file into this process */
	rc = elf_load_binary(info->argv[0], info->mmu, &entry);
	if (rc != 0) {
		DEBUG(DL_DBG, ("elf_load_binary failed, err(%d).\n",
			       rc));
		return;
	}

	/* We use a fixed user stack address for now */
	CURR_THREAD->ustack = (void *)(USTACK_BOTTOM + USTACK_SIZE);
	
	/* Switch to user space */
	arch_thread_enter_uspace(entry, (void *)(USTACK_BOTTOM + USTACK_SIZE));
}

static struct process_creation *alloc_process_creation(const char *args[])
{
	int i;
	char *ptr;
	size_t size;
	struct process_creation *info;

	info = kmalloc(sizeof(struct process_creation), 0);
	if (!info) {
		goto out;
	}
	memset(info, 0, sizeof(struct process_creation));

	for (i = 0, size = 0; args[i] != NULL; i++) {
		size += strlen(args[i]) + 1;
	}
	
	info->argc = i;
	size += sizeof(char *) * info->argc + 1;
	
	info->argv = kmalloc(size, 0);
	if (!info->argv) {
		kfree(info);
		info = NULL;
		goto out;
	}

	for (i = 0, ptr = (char *)&info->argv[info->argc]; i < info->argc; i++) {
		strcpy(ptr, args[i]);
		info->argv[i] = ptr;
		ptr += (strlen(ptr) + 1);
	}

out:
	return info;
}

int process_create(const char *args[], struct process *parent, int flags,
		   int priority, struct process **procp)
{
	int rc = -1;
	struct process *p = NULL;
	struct mmu_ctx *mmu = NULL;
	struct thread *t = NULL;
	struct process_creation *info;

	if (!args || !args[0] || priority >= 32) {
		DEBUG(DL_DBG, ("invalid parameter.\n"));
		rc = -1;
		goto out;
	}

	memset(&info, 0, sizeof(struct process_creation));
	
	/* Create the address space for the process */
	mmu = mmu_create_ctx();
	if (!mmu) {
		DEBUG(DL_INF, ("mmu_create_ctx failed.\n"));
		rc = -1;
		goto out;
	}
	mmu_copy_ctx(mmu, &_kernel_mmu_ctx);

	info = alloc_process_creation(args);
	info->mmu = mmu;

	/* Create the new process */
	rc = process_alloc(args[0], parent, mmu, flags, priority, NULL, &p);
	if (rc != 0) {
		DEBUG(DL_INF, ("process_alloc failed, err(%d).\n", rc));
		goto out;
	}

	/* Create the main thread and run it */
	rc = thread_create("main", p, 0, process_entry_thread, info, &t);
	if (rc != 0) {
		DEBUG(DL_INF, ("thread_create failed, err(%d).\n", rc));
		goto out;
	}

	p->creation = info;
	thread_run(t);
	thread_release(t);

	if (procp) {
		*procp = p;
	}
	rc = 0;		// We'are OK

out:
	if (rc != 0) {
		if (p) {
			process_destroy(p);
		}
		if (mmu) {
			mmu_destroy_ctx(mmu);
		}
	}

	return rc;
}

int process_destroy(struct process *proc)
{
	ASSERT(LIST_EMPTY(&proc->threads));
	
	/* Remove this process from the process tree */
	avl_tree_remove_node(&_proc_tree, &proc->tree_link);

	notifier_clear(&proc->death_notifier);
	
	kfree(proc);

	return 0;
}

static void process_wait_notifier(void *ctx)
{
	struct semaphore *s;

	s = (struct semaphore *)ctx;
	semaphore_up(s, 1);
}

int process_wait(struct process *p, void *sync)
{
	int rc = -1;

	DEBUG(DL_DBG, ("process(%s:%d:%d)\n", p->name, p->id, p->state));
	
	if (p->state != PROCESS_DEAD) {
		/* Register us to the death notifier of the specified process */
		notifier_register(&p->death_notifier, process_wait_notifier, sync);
		rc = 0;
	} else {
		DEBUG(DL_INF, ("process(%s:%d) dead.", p->name, p->id));
		rc = -1;
	}

	return rc;
}

int process_clone(void (*entry)(void *), void *esp, void *args, struct process **procp)
{
	int rc = -1;
	struct mmu_ctx *mmu;
	struct thread *t = NULL;
	struct process *p = NULL;
	struct fd_table *fds = NULL;
	struct thread_uspace_creation *info;
	
	/* Clone the parent(current)'s page directory */
	mmu = mmu_create_ctx();
	if (!mmu) {
		DEBUG(DL_DBG, ("mmu_create_ctx failed.\n"));
		goto out;
	}
	mmu_copy_ctx(mmu, CURR_PROC->mmu_ctx);

	/* Clone the parent(current)'s file descriptor table */
	fds = fd_table_clone(CURR_PROC->fds);
	if (!fds) {
		DEBUG(DL_DBG, ("fd_table_clone failed.\n"));
		goto out;
	}

	/* Create the new process and a handle */
	rc = process_alloc(CURR_PROC->name, CURR_PROC, mmu, 0, 16, fds, &p);
	if (rc != 0) {
		DEBUG(DL_DBG, ("process_alloc failed, err(%d).\n", rc));
		goto out;
	}

	/* Create the main thread and run it */
	info = kmalloc(sizeof(struct thread_uspace_creation), 0);
	info->entry = entry;
	info->esp = esp;
	info->args = args;
	
	rc = thread_create("main", p, 0, thread_uspace_trampoline, info, &t);
	if (rc != 0) {
		DEBUG(DL_DBG, ("thread_create failed, err(%d).\n", rc));
		goto out;
	}

	*procp = p;

	/* Run the new thread */
	thread_run(t);
	thread_release(t);

out:
	if (rc != 0) {
		if (p) {
			process_destroy(p);
		}
		if (fds) {
			fd_table_destroy(fds);
		}
		if (mmu) {
			/* TODO: Should unmap mmu context */
			mmu_destroy_ctx(mmu);
		}
	}
	
	return rc;
}

int process_replace(const char *path, const char *args[])
{
	return -1;
}

int process_getid()
{
	return CURR_PROC->id;
}

/**
 * Start our kernel at top half
 */
void init_process()
{
	int rc;
	
	/* Relocate the stack so we know where it is, the stack size is 8KB. Note
	 * that this was done in the context of kernel mmu.
	 */
	move_stack(0xE0000000, KSTACK_SIZE);

	/* Initialize the process avl tree */
	avl_tree_init(&_proc_tree);

	/* Create the kernel process */
	rc = process_alloc("kernel", NULL, NULL, 0, 16, NULL, &_kernel_proc);
	if (rc != 0) {
		PANIC("Could not initialize kernel process");
	}

	DEBUG(DL_DBG, ("allocated kernel process(%p).\n", _kernel_proc));
}
