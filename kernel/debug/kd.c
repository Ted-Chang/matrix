#include <types.h>
#include <stddef.h>
#include <stdarg.h>
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "list.h"
#include "atomic.h"
#include "hal/cpu.h"
#include "hal/spinlock.h"
#include "mm/page.h"
#include "dbgheap.h"
#include "kd.h"
#include "platform.h"

/* Kernel debugger heap size */
#define KD_HEAP_SIZE	0x4000

/* KD command descriptor */
struct kd_cmd_desc {
	struct list link;		// Link to the command list
	const char *name;		// Name of the command
	const char *description;	// Description of the command
	kd_cmd_t func;			// Command handler function
};
typedef struct kd_cmd_desc kd_cmd_desc_t;

/* Whether KD is currently running on any CPU */
atomic_t _kd_running = 0;

/* Statically allocated heap for use within KD */
static u_char _kd_heap_area[KD_HEAP_SIZE]__attribute__((aligned(PAGE_SIZE)));
static struct dbg_heap _kd_heap;

/* List of registered commands */
static struct list _kd_cmds = {
	.prev = &_kd_cmds,
	.next = &_kd_cmds
};
static struct spinlock _kd_cmds_lock;

void arch_init_kd()
{
	// Do nothing here
}

void kd_vprintf(const char *fmt, va_list args)
{
	;
}

void kd_printf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	kd_vprintf(fmt, args);
	va_end(args);
}

void *kd_malloc(size_t size)
{
	void *ret;

	ret = dbg_heap_alloc(&_kd_heap, size);
	if (!ret) {
		if (_kd_running) {
			kd_printf("Exhausted KD heap");
			cpu_halt();
		} else {
			PANIC("Exhausted KD heap");
		}
	}

	return ret;
}

void kd_free(void *addr)
{
	dbg_heap_free(&_kd_heap, addr);
}

static int kd_cmd_help(int argc, char **argv, kd_filter_t *filter)
{
	struct kd_cmd_desc *cmd;
	struct list *l;

	LIST_FOR_EACH(l, &_kd_cmds) {
		cmd = LIST_ENTRY(l, struct kd_cmd_desc, link);
		kd_printf("%s %s", cmd->name, cmd->description);
	}
	
	return 0;
}

static int kd_cmd_reboot(int argc, char **argv, kd_filter_t *filter)
{
	platform_reboot();
	return 0;
}

void kd_register_cmd(const char *name, const char *desc, kd_cmd_t func)
{
	struct kd_cmd_desc *cmd, *exist;
	struct list *l;
	int rc;

	spinlock_acquire(&_kd_cmds_lock);

	cmd = kd_malloc(sizeof(struct kd_cmd_desc));
	LIST_INIT(&cmd->link);
	cmd->name = name;
	cmd->description = desc;
	cmd->func = func;

	/* Keep the command list sorted alphabetically */
	LIST_FOR_EACH(l, &_kd_cmds) {
		exist = LIST_ENTRY(l, struct kd_cmd_desc, link);
		rc = strcmp(name, exist->name);
		if (rc == 0) {	// Already registered
			kd_free(cmd);
			spinlock_release(&_kd_cmds_lock);
			return;
		} else if (rc < 0) {
			break;
		}
		
	}

	list_add(&cmd->link, l->prev);
	
	spinlock_release(&_kd_cmds_lock);
}

void kd_unregister_cmd(const char *name)
{
	struct kd_cmd_desc *cmd;
	struct list *l;

	spinlock_acquire(&_kd_cmds_lock);

	LIST_FOR_EACH(l, &_kd_cmds) {
		cmd = LIST_ENTRY(l, struct kd_cmd_desc, link);
		if (strcmp(name, cmd->name) == 0) {
			list_del(&cmd->link);
			kd_free(cmd);
			break;
		}
	}

	spinlock_release(&_kd_cmds_lock);
}

void init_kd()
{
	/* Initialize the kd commands lock */
	spinlock_init(&_kd_cmds_lock, "kd-cmds-lock");
	
	/* Initialize the debug heap */
	init_dbg_heap(&_kd_heap, _kd_heap_area, KD_HEAP_SIZE);

	/* Register architecture-specific commands */
	arch_init_kd();

	/* Register our own commands */
	kd_register_cmd("help", "Display usage information for KD commands.", kd_cmd_help);
	kd_register_cmd("reboot", "Reboot the system.", kd_cmd_reboot);
}
