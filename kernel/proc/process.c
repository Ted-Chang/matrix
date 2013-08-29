/*
 * process.c
 */

#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/matrix.h"
#include "matrix/const.h"
#include "matrix/process.h"
#include "hal/hal.h"
#include "mm/mlayout.h"
#include "mm/page.h"
#include "mm/mmu.h"
#include "mm/malloc.h"
#include "mm/slab.h"
#include "kstrdup.h"
#include "rtl/object.h"
#include "proc/process.h"
#include "proc/sched.h"
#include "proc/thread.h"
#include "debug.h"
#include "elf.h"
#include "semaphore.h"

struct process_creation {
	struct semaphore sem;	// Semaphore for synchronize

	int argc;		// Argument count
	const char **argv;	// Arguments
	const char **env;	// Environments

	/* Information used internally by the loader */
	struct va_space *vas;	// Address space for the process
	void *data;		// Data pointer for the ELF loader
	ptr_t args;		// Address for argument block mapping
	ptr_t ustack;		// Address for stack mapping

	/* Information to return to the caller */
	int status;		// Status code to return from the call
};

static pid_t _next_pid = 1;

/* Process structure cache */
static slab_cache_t _proc_cache;

/* Tree of all processes */
static struct avl_tree _proc_tree;
static struct mutex _proc_tree_lock;

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
	int rc;
	uint32_t i, pd_addr;
	uint32_t old_esp, old_ebp;
	uint32_t new_esp, new_ebp;
	uint32_t offset;
	ptr_t start;

	start = new_stack - size;
	rc = mmu_map(&_kernel_mmu_ctx, start, size + PAGE_SIZE,
		     MAP_READ_F|MAP_WRITE_F|MAP_FIXED_F);
	ASSERT(rc == 0);
	
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

	DEBUG(DL_DBG, ("o_esp(0x%x) o_ebp(0x%x) n_esp(0x%x) n_ebp(0x%x) i_esp(0x%x)\n",
		       old_esp, old_ebp, new_esp, new_ebp, _initial_esp));
	
	/* Copy the old stack to new stack. Although we have switched to cloned
	 * page directory, the kernel memory map was not changed. So we can just
	 * copy the data between the initial esp and the old esp to our new stack.
	 */
	memcpy((void *)new_esp, (void *)old_esp, _initial_esp - old_esp);

	/* Backtrace through the original stack, copying new values into the
	 * new stack. This is just stack relocation.
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
	if (p->vas) {
		va_destroy(p->vas);
		p->vas = NULL;
	}
	
	if (p->fds) {
		fd_table_destroy(p->fds);
		p->fds = NULL;
		io_destroy_ctx(&p->ioctx);
	}
}

static int process_alloc(const char *name, struct process *parent, struct va_space *vas,
			 int flags, int priority, struct fd_table *fds,
			 struct process **procp)
{
	int rc = -1;
	struct process *p;

	ASSERT((name != NULL) && (procp != NULL));

	/* Allocate the new process structure from slab allocator */
	p = slab_cache_alloc(&_proc_cache);
	if (!p) {
		DEBUG(DL_DBG, ("slab allocate process failed.\n"));
		rc = -1;
		goto out;
	}

	/* Allocate a process ID. If creating the kernel process, always give
	 * it an ID of 0.
	 */
	if (_kernel_proc) {
		p->id = id_alloc();	// Allocate an ID for the process
	} else {
		p->id = 0;		// The first process must be kernel process
	}
	
	p->name = kstrdup(name, 0);

	p->uid = 500;			// FixMe: set it to the currently logged user
	p->gid = 500;			// FixMe: set it to the current user's group
	p->vas = vas;			// Virtual address space
	p->priority = priority;
	p->flags = flags;
	p->status = 0;
	
	io_init_ctx(&p->ioctx, parent ? &parent->ioctx : NULL);
	
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

	/* If this process is a clone of other inherite signal handling information
	 * from the parent
	 */
	if (FLAG_ON(flags, PROCESS_CLONE_F)) {
		ASSERT(parent != NULL);
		memcpy(p->signal_act, parent->signal_act, sizeof(p->signal_act));
		p->signal_mask = parent->signal_mask;
	} else {
		memset(p->signal_act, 0, sizeof(p->signal_act));
		p->signal_mask = 0;
	}

	/* Insert this process into process tree */
	mutex_acquire(&_proc_tree_lock);
	avl_tree_insert_node(&_proc_tree, &p->tree_link, p->id, p);
	mutex_release(&_proc_tree_lock);

	p->state = PROCESS_RUNNING;
	*procp = p;
	rc = 0;

	DEBUG(DL_DBG, ("allocated process(%s:%d:%p:%p).\n",
		       p->name, p->id, p, p->vas));
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

	mutex_acquire(&_proc_tree_lock);
	proc = avl_tree_lookup(&_proc_tree, pid);
	mutex_release(&_proc_tree_lock);

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

	DEBUG(DL_DBG, ("process(%s:%d), thread number(%d).\n", CURR_PROC->name,
		       CURR_PROC->id, n));
	
	CURR_PROC->status = status;
	thread_exit();

	return 0;
}

static int create_aspace(struct process_creation *info)
{
	int rc = -1, i;
	size_t size;
	void *data = NULL;
	struct vfs_node *n;
	struct va_space *vas = NULL;

	/* Initialize the semaphore for synchronize with new entry thread */
	semaphore_init(&info->sem, "p-create-sem", 0);
	
	/* Create the address space for the process */
	vas = va_create();
	if (!vas) {
		DEBUG(DL_INF, ("va_create failed.\n"));
		rc = -1;
		goto out;
	}
	mmu_clone_ctx(vas->mmu, &_kernel_mmu_ctx);

	/* Lookup the file from the file system */
	n = vfs_lookup(info->argv[0], VFS_FILE);
	if (!n) {
		DEBUG(DL_DBG, ("file(%s) not found.\n", info->argv[0]));
		rc = -1;
		goto out;
	}

	/* Load the ELF file into this process */
	rc = elf_load_binary(n, vas, &data);
	if (rc != 0) {
		DEBUG(DL_DBG, ("elf_load_binary failed, err(%d).\n", rc));
		goto out;
	}

	/* Calculate the size of arguments */
	for (i = 0, size = 0; info->argv[i] != NULL; i++) {
		size += (strlen(info->argv[i]) + 1);
		DEBUG(DL_DBG, (" argv[%d] - \"%s\"\n", i, info->argv[i]));
	}
	size += ((sizeof(char *) * i) + 1 + sizeof(struct process_args));
	size = ROUND_UP(size, PAGE_SIZE);
	info->argc = i;

	/* Map some pages for the user mode stack from the new mmu context */
	rc = va_map(vas, USTACK_BOTTOM, USTACK_SIZE,
		    VA_MAP_READ|VA_MAP_WRITE|VA_MAP_FIXED, NULL);
	if (rc != 0) {
		DEBUG(DL_DBG, ("va_map for ustack failed, err(%d).\n", rc));
		goto out;
	}

	/* Map some pages for the arguments block after the stack, leave one page
	 * non-allocated to probe stack underflow
	 */
	info->args = USTACK_BOTTOM + USTACK_SIZE + PAGE_SIZE;
	rc = va_map(vas, info->args, size,
		    VA_MAP_READ|VA_MAP_WRITE|VA_MAP_FIXED, NULL);
	if (rc != 0) {
		DEBUG(DL_DBG, ("va_map for arguments failed, err(%d).\n", rc));
		goto out;
	}
	
	info->vas = vas;
	info->ustack = USTACK_BOTTOM;
	info->data = data;
	info->status = rc;

 out:
	if (rc != 0) {
		if (vas) {
			va_destroy(vas);
		}
	}
	
	return rc;
}

static void copy_process_args(const char *src[], size_t count, ptr_t dst)
{
	size_t i;
	char *ptr;
	struct process_args *args;

	args = (struct process_args *)dst;
	args->argc = count;
	args->argv = (char **)dst + sizeof(struct process_args);
	ptr = (char *)&args->argv[count];

	for (i = 0; i < count; i++) {
		strcpy(ptr, src[i]);
		args->argv[i] = ptr;
		ptr += (strlen(ptr) + 1);
	}
}

static void process_entry_thread(void *ctx)
{
	ptr_t ustack, entry, args;
	struct process_creation *info;

	info = (struct process_creation *)ctx;

	ASSERT(CURR_ASPACE == info->vas);

	/* We use a fixed user stack address for now */
	ustack = info->ustack + USTACK_SIZE - 1;
	args = (ptr_t)info->args;

	/* Copy the arguments */
	copy_process_args(info->argv, info->argc, args);

	/* Get the ELF loader to clear BSS and get the entry pointer */
	entry = elf_finish_binary(info->data);

	CURR_THREAD->ustack = (void *)info->ustack;
	CURR_THREAD->ustack_size = USTACK_SIZE;

	DEBUG(DL_DBG, ("ustack(%p), args(%p).\n", ustack, args));
	
	semaphore_up(&info->sem, 1);
	
	/* Switch to user space */
	arch_thread_enter_uspace(entry, ustack, args);
	
	PANIC("Failed to enter user space");
}

int process_create(const char **args, struct process *parent, int flags,
		   int priority, struct process **procp)
{
	int rc = -1;
	struct process *p = NULL;
	struct thread *t = NULL;
	struct process_creation info;

	if (!args || !args[0] || priority >= 32) {
		DEBUG(DL_DBG, ("invalid parameter.\n"));
		rc = -1;
		goto out;
	}

	memset(&info, 0, sizeof(struct process_creation));
	
	info.argv = args;
	info.env = NULL;

	/* Create MMU context and map pages need for the new process  */
	rc = create_aspace(&info);
	if (rc != 0) {
		DEBUG(DL_INF, ("create_aspace failed, err(%d).\n", rc));
		goto out;
	}

	/* Create the new process */
	rc = process_alloc(args[0], parent, info.vas, flags, priority, NULL, &p);
	if (rc != 0) {
		DEBUG(DL_INF, ("process_alloc failed, err(%d).\n", rc));
		goto out;
	}

	/* Create the main thread and run it */
	rc = thread_create("main", p, 0, process_entry_thread, &info, &t);
	if (rc != 0) {
		DEBUG(DL_INF, ("thread_create failed, err(%d).\n", rc));
		goto out;
	}

	p->creation = &info;
	thread_run(t);
	thread_release(t);

	/* Wait for the process to finish loading */
	semaphore_down(&info.sem);
	if (procp) {
		*procp = p;
	}
	rc = info.status;	// We'are OK

 out:
	if (rc != 0) {
		if (p) {
			process_destroy(p);
		}
		if (info.vas) {
			va_destroy(info.vas);
		}
	}

	return rc;
}

int process_destroy(struct process *proc)
{
	ASSERT(LIST_EMPTY(&proc->threads));
	
	/* Remove this process from the process tree */
	mutex_acquire(&_proc_tree_lock);
	avl_tree_remove_node(&_proc_tree, &proc->tree_link);
	mutex_release(&_proc_tree_lock);

	notifier_clear(&proc->death_notifier);

	kfree(proc->name);
	
	/* Free this process to our process cache */
	slab_cache_free(&_proc_cache, proc);

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
		DEBUG(DL_INF, ("process(%s:%d) dead.\n", p->name, p->id));
		rc = -1;
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
	int rc = -1;
	
	/* Relocate the stack so we know where it is, the stack size is 8KB. Note
	 * that this was done in the context of kernel mmu.
	 */
	move_stack(0xE0000000, KSTACK_SIZE);

	/* Initialize the process slab cache */
	slab_cache_init(&_proc_cache, "proc-cache", sizeof(struct process),
			process_ctor, process_dtor, 0);

	/* Initialize the process avl tree and its lock */
	avl_tree_init(&_proc_tree);
	mutex_init(&_proc_tree_lock, "ptree-mutex", 0);

	/* Create the kernel process. Note that kernel process doesn't need virtual
	 * address space.
	 */
	rc = process_alloc("kernel", NULL, NULL, 0, 16, NULL, &_kernel_proc);
	if (rc != 0) {
		PANIC("Could not initialize kernel process");
	}

	DEBUG(DL_DBG, ("allocated kernel process(%p).\n", _kernel_proc));
}
