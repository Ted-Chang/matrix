#include <types.h>
#include "cpu.h"
#include "mmgr.h"

static size_t _nr_cpus;
static size_t _highest_cpu_id;

cpu_id_t cpu_id()
{
	return 0;
}

void init_cpu()
{
	/* Get the ID of the boot CPU. */
	_highest_cpu_id = cpu_id();
	_nr_cpus = 1;
}
