#include <types.h>
#include <stddef.h>
#include <string.h>
#include "debug.h"
#include "list.h"
#include "hal/hal.h"
#include "hal/core.h"
#include "hal/lapic.h"
#include "mm/mlayout.h"
#include "mm/mmu.h"
#include "mm/kmem.h"
#include "mm/malloc.h"
#include "platform.h"
#include "smp.h"
#include "pit.h"

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

/* Page reserved to copy the AC bootstrap code to */
static phys_addr_t _ac_bootstrap_page = 0;

static struct smp_call *_smp_call_pool = NULL;
static boolean_t _smp_call_enabled = FALSE;

/* Variable used to synchronize the stages of the SMP boot process */
volatile uint32_t _smp_boot_status = 0;

extern char __ac_trampoline_start[], __ac_trampoline_end[];
extern void kmain_ac(struct core *c);

void arch_smp_boot_prepare()
{
	void *mapping;
	
	/* Allocate a low memory page for the trampoline code. At this time
	 * the application core is in real mode and only can access memory
	 * lower than 1MB. Also the AC will start execution from 0x000VV000.
	 */
	mapping = (void *)0x00090000;	// FixMe: we are using a fixed addresss for
	_ac_bootstrap_page = 0x00090000;// now as we have identity mapped the memory.
					// You should do physical alloc.
	
	ASSERT(__ac_trampoline_end - __ac_trampoline_start < PAGE_SIZE);
	DEBUG(DL_DBG, ("start(%x), end(%x).\n", __ac_trampoline_start, __ac_trampoline_end));
	memcpy(mapping, __ac_trampoline_start, __ac_trampoline_end - __ac_trampoline_start);
}

static boolean_t boot_core_and_wait(core_id_t id)
{
	boolean_t ret = FALSE;
	//useconds_t delay = 0;
	
	/* Send an INIT IPI to the AC to reset its state and delay 10ms */
	lapic_ipi(LAPIC_IPI_DEST_SINGLE, id, LAPIC_IPI_INIT, 0x00);
	spin(10000);

	/* Send a Start-up IPI. The vector argument specifies where to look
	 * for the bootstrap code. As the SIPI will start execution from 0x000VV000,
	 * where VV is the vector specified in the IPI.
	 */
	lapic_ipi(LAPIC_IPI_DEST_SINGLE, id, LAPIC_IPI_SIPI, _ac_bootstrap_page >> 12);
	spin(10000);

	/* If the CORE is up then return */
	if (_smp_boot_status > SMP_BOOT_INIT) {
		ret = TRUE;
		goto out;
	}

	/* Sends a second Start-up IPI and then check in 10ms intervals to
	 * see if it has booted. If it hasn't booted after 5 seconds, fail.
	 */
	/* lapic_ipi(LAPIC_IPI_DEST_SINGLE, id, LAPIC_IPI_SIPI, _ac_bootstrap_page >> 12); */
	/* for (delay = 0; delay < 500000; delay += 10000) { */
	/* 	if (_smp_boot_status > SMP_BOOT_INIT) { */
	/* 		ret = TRUE; */
	/* 		goto out; */
	/* 	} */
	/* 	spin(10000); */
	/* } */

	ret = TRUE;

 out:
	return ret;
}

void arch_smp_boot_core(struct core *c)
{
	kprintf("smp: booting CORE %d...\n", c->id);
	ASSERT(lapic_enabled());

	/* Allocate a double fault stack for the new CORE. This is also
	 * used as the initial stack while initializing the AC, before it
	 * enters the scheduler.
	 */
	c->arch.double_fault_stack = kmem_alloc(KSTACK_SIZE, 0);
	ASSERT(c->arch.double_fault_stack != NULL);

	/* Fill in details required by the bootstrap code */
	/* (*(uint32_t *)(mapping + 16)) = (ptr_t)kmain_ac; */
	/* (*(uint32_t *)(mapping + 20)) = (ptr_t)c; */
	/* (*(uint32_t *)(mapping + 24)) = (ptr_t)c->arch.double_fault_stack + KSTACK_SIZE; */
	/* (*(uint32_t *)(mapping + 28)) = (ptr_t)_ac_mmu_ctx->pdbr; */

	/* Wakeup the CORE */
	if (!boot_core_and_wait(c->id)) {
		DEBUG(DL_ERR, ("boot CORE %d failed.\n", c->id));
		goto out;
	}

	/* Sync the TSC of the AC with the boot CORE */
	tsc_init_source();

 out:
	return;
}

void arch_smp_boot_cleanup()
{
	/* Free the bootstrap page */
}

void smp_boot_cores()
{
	core_id_t i;
	boolean_t state;

	state = local_irq_disable();
	arch_smp_boot_prepare();

	for (i = 0; i <= _highest_core_id; i++) {
		if (_cores[i] && _cores[i]->state == CORE_OFFLINE) {
			arch_smp_boot_core(_cores[i]);
		}
	}

	arch_smp_boot_cleanup();

	local_irq_restore(state);
}

void smp_ipi_handler()
{
	ASSERT(_smp_call_enabled);

	DEBUG(DL_DBG, ("received IPI.\n"));
}

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

	/* Boot all the application COREs in the system */
	smp_boot_cores();

 out:
	return;
}

