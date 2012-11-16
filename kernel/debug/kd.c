#include <types.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
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
#include "console.h"

/* Kernel debugger heap size */
#define KD_HEAP_SIZE	0x4000

/* Maximum number of arguments to a function */
#define KD_MAX_ARGS	16

/* KD command descriptor */
struct kd_cmd_desc {
	struct list link;		// Link to the command list
	const char *name;		// Name of the command
	const char *description;	// Description of the command
	kd_cmd_t func;			// Command handler function
};
typedef struct kd_cmd_desc kd_cmd_desc_t;

/* KD command argument */
struct kd_args {
	struct list link;		// For internal use
	int count;			// Number of arguments
	char *args[KD_MAX_ARGS];	// Array of arguments
};
typedef struct kd_args kd_args_t;

/* Structure containing parsed line information */
struct kd_line {
	struct kd_args call;		// Primary command call
	struct list filters;		// List of filter commands
	int filter_count;		// Number of filters
};
typedef struct kd_line kd_line_t;

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

static kd_cmd_desc_t *lookup_cmd(const char *name)
{
	kd_cmd_desc_t *cmd;
	struct list *l;

	LIST_FOR_EACH(l, &_kd_cmds) {
		cmd = LIST_ENTRY(l, kd_cmd_desc_t, link);
		if (strcmp(name, cmd->name) == 0) {
			return cmd;
		}
	}

	return NULL;
}

static void kd_putc(char ch)
{
	if (_debug_console_ops) {
		_debug_console_ops->putc(ch);
	}
}

static void kd_printf_helper(const char *str, size_t size)
{
	size_t i;
	
	for (i = 0; i < size; i++) {
		kd_putc(str[i]);
	}
}

static void kd_vprintf(const char *fmt, va_list args)
{
	do_printf(kd_printf_helper, fmt, args);
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

static char *kd_read_line(int count)
{
	return NULL;
}

static boolean_t kd_parse_line(char *line, kd_line_t *data)
{
	return FALSE;
}

int perform_call(kd_args_t *call, kd_filter_t *filter, kd_filter_t *filter_arg)
{
	int rc;
	kd_cmd_desc_t *cmd;

	/* Look up the command */
	cmd = lookup_cmd(call->args[0]);
	if (!cmd) {
		kd_printf("KD: Unknown command %s\n", call->args[0]);
		return -1;
	}

	/* Set _kd_running to 2 to signal that we're in a command */
	_kd_running = 2;
	
	rc = -1;

	_kd_running = 1;
	return rc;
}

int kd_main(int reason, u_long index)
{
	static u_long _nr_cmds = 0;
	int rc = 0;
	boolean_t state;
	char *line;
	kd_line_t data;
	kd_filter_t *filter;
	
	/* Disable the interrupts while we're running */
	state = irq_disable();

	/* Check if we're already running. This shouldn't happen */
	if (!atomic_tas(&_kd_running, 0, 1)) {
		kd_printf("Multiple entries to KD.\n");
		irq_restore(state);
		return -1;
	}

	/* Main loop, get and process the input */
	while (TRUE) {
		line = kd_read_line(_nr_cmds++);
		if (!line) {
			kd_printf("KD: Please enter a command.\n");
			continue;
		}

		/* Parse the line */
		if (!kd_parse_line(line, &data)) {
			continue;
		}

		/* Setup the filter if any */
		if (data.filter_count) {
			if (data.filter_count > 1) {
				kd_printf("KD: Multiple filters not supported.\n");
				continue;
			}
		} else {
			filter = NULL;
		}

		/* Perform the main call */
		rc = perform_call(&data.call, filter, NULL);
		if (filter) {
			;
		}

		/* Handle the return code */
		if (rc == 0) {
			;
		}
	}

	_kd_running = 0;
	irq_restore(state);

	return rc;
}

static int kd_cmd_help(int argc, char **argv, kd_filter_t *filter)
{
	kd_cmd_desc_t *cmd;
	struct list *l;

	LIST_FOR_EACH(l, &_kd_cmds) {
		cmd = LIST_ENTRY(l, kd_cmd_desc_t, link);
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
	kd_cmd_desc_t *cmd, *exist;
	struct list *l;
	int rc;

	spinlock_acquire(&_kd_cmds_lock);

	cmd = kd_malloc(sizeof(kd_cmd_desc_t));
	LIST_INIT(&cmd->link);
	cmd->name = name;
	cmd->description = desc;
	cmd->func = func;

	/* Keep the command list sorted alphabetically */
	LIST_FOR_EACH(l, &_kd_cmds) {
		exist = LIST_ENTRY(l, kd_cmd_desc_t, link);
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
		cmd = LIST_ENTRY(l, kd_cmd_desc_t, link);
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
