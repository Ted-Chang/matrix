#include <types.h>
#include <stddef.h>
#include "kd.h"

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

	/* Register our own commands */
	kd_register_cmd("help", "Display usage information for KD commands.", kd_cmd_help);
}
