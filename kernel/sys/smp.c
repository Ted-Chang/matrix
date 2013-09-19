#include <types.h>
#include <stddef.h>
#include <string.h>
#include "debug.h"
#include "list.h"
#include "mm/malloc.h"
#include "platform.h"
#include "smp.h"

#define SMP_CALLS_PER_CORE	4

/* SMP call information */
struct smp_call {
	struct list link;	// Link to the destination CORE's call queue
	struct smp_call *next;	// Pointer to the next structure in the free pool

	smp_call_func_t func;	// Handler function
	void *ctx;		// Argument to handler
	
	int status;		// Status code function returned
	int ref_count;		// Reference count
};

static struct smp_call *_smp_call_pool = NULL;
static boolean_t _smp_call_enabled = FALSE;

void init_smp()
{
	size_t cnt, i;
	struct smp_call *calls;
	
	/* Detect application CORE */
	platform_detect_smp();

	/* If we have only 1 CORE, there's nothing to do */
	if (_nr_cores == 1) {
		DEBUG(DL_DBG, ("Only 1 core.\n"));
		goto out;
	}

	/* Allocate message structures based on the total CORE count */
	cnt = _nr_cores * SMP_CALLS_PER_CORE;
	calls = kmalloc(cnt * sizeof(struct smp_call), 0);
	if (!calls) {
		goto out;
	}
	memset(calls, 0, cnt * sizeof(struct smp_call));

	/* Initialize each structure and add it to the pool */
	for (i = 0; i < cnt; i++) {
		LIST_INIT(&calls[i].link);
		calls[i].next = _smp_call_pool;
		_smp_call_pool = &calls[i];
	}

	_smp_call_enabled = TRUE;

 out:
	return;
}
