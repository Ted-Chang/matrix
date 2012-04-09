#include <types.h>
#include <stddef.h>
#include "list.h"
#include "mm/page.h"
#include "mm/mmgr.h"
#include "mm/fixed_heap.h"
#include "kd.h"

#define KD_HEAP_SIZE	0x4000

/* Statically allocated heap for use within Kernel Debugger */
static u_char kd_heap_area[KD_HEAP_SIZE]__attribute__((aligned(PAGE_SIZE)));
static struct fixed_heap _kd_heap;

static kd_status_t kd_cmd_help(int argc, char **argv, struct kd_filter *filter)
{
	return KD_SUCCESS;
}

static kd_status_t kd_cmd_continue(int argc, char **argv, struct kd_filter *filter)
{
	return KD_CONTINUE;
}

static kd_status_t kd_cmd_step(int argc, char **argv, struct kd_filter *filter)
{
	return KD_STEP;
}

static kd_status_t kd_cmd_reboot(int argc, char **argv, struct kd_filter *filter)
{
	return KD_FAILURE;
}

static kd_status_t kd_cmd_regs(int argc, char **argv, struct kd_filter *filter)
{
	return KD_SUCCESS;
}

static kd_status_t kd_cmd_examine(int argc, char **argv, struct kd_filter *filter)
{
	return KD_SUCCESS;
}

static kd_status_t kd_cmd_print(int argc, char **argv, struct kd_filter *filter)
{
	return KD_SUCCESS;
}

static kd_status_t kd_cmd_backtrace(int argc, char **argv, struct kd_filter *filter)
{
	return KD_SUCCESS;
}

static kd_status_t kd_cmd_grep(int argc, char **argv, struct kd_filter *filter)
{
	return KD_SUCCESS;
}

void kd_register_cmd(const char *name, const char *desc, kd_cmd_t func)
{
	int rc;
}

void kd_unregister_cmd(const char *name)
{
	;
}

void init_kd()
{
	/* Initialize a fixed heap for the kernel debugger */
	init_fixed_heap(&_kd_heap, kd_heap_area, KD_HEAP_SIZE);
	
	/* Register our own commands */
	kd_register_cmd("help", "Display usage information for KDB commands.",
			kd_cmd_help);
	kd_register_cmd("continue", "Exit KDB and continue execution.",
			kd_cmd_continue);
	kd_register_cmd("step", "Single-step over instructions.",
			kd_cmd_step);
	kd_register_cmd("reboot", "Forcibly reboot the system.",
			kd_cmd_reboot);
	kd_register_cmd("regs", "Print the values of all CPU registers.",
			kd_cmd_regs);
	kd_register_cmd("examine", "Examine the contents of memory.",
			kd_cmd_examine);
	kd_register_cmd("print", "Print out the value of an expression.",
			kd_cmd_print);
	kd_register_cmd("backtrace", "Print out a backtrace.",
			kd_cmd_backtrace);
	kd_register_cmd("grep", "Search the output of commands.",
			kd_cmd_grep);
}
