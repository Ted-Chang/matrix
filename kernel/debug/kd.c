#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "list.h"
#include "mm/page.h"
#include "kd.h"

/* Kernel debugger heap size */
#define KD_HEAP_SIZE	0x4000

/* Statically allocated heap for use within KD */
static u_char _kd_heap[KD_HEAP_SIZE]__attribute__((aligned(PAGE_SIZE)));

/* List of registered commands */
static struct list _kd_commands = {
	.prev = &_kd_commands,
	.next = &_kd_commands
};

static int kd_cmd_help(int argc, char **argv, kd_filter_t *filter)
{
	return 0;
}

void kd_register_cmd(const char *name, const char *desc, kd_cmd_t func)
{
	;
}

void kd_unregister_cmd(const char *name)
{
	;
}

void kd_init()
{
	/* Initialize the fixed heap */
	;

	/* Register architecture-specific commands */
	;

	/* Register our own commands */
	kd_register_cmd("help", "Display usage information for KD commands.", kd_cmd_help);
}
