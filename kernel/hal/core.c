#include <types.h>
#include <string.h>
#include "hal/hal.h"
#include "hal/lapic.h"
#include "hal/core.h"
#include "pit.h"
#include "debug.h"
#include "mm/mlayout.h"
#include "mm/page.h"
#include "mm/malloc.h"
#include "div64.h"

#define FREQ_ATTEMPTS	9

extern struct idt_ptr _idt_ptr;
extern void idt_flush();
extern void init_idt();
extern void init_gdt(struct core *c);
extern void init_tss(struct core *c);

/* Feature set present on all COREs */
struct core_features _core_features;

/* Boot CORE structure */
struct core _boot_core;

/* Information about all COREs */
size_t _nr_cores;
size_t _highest_core_id;

struct list _running_cores = {
	.prev = &_running_cores,
	.next = &_running_cores,
};

struct core **_cores = NULL;

/* Double fault handler stack for the boot CORE */
static u_char _boot_double_fault_stack[KSTACK_SIZE]__attribute__((aligned(PAGE_SIZE)));

core_id_t core_id()
{
	return (core_id_t)lapic_id();
}

static void init_descriptor(struct core *c)
{
	/* Initialize and load GDT/TSS */
	init_gdt(c);
	init_tss(c);

	/* Point this CORE to the global IDT */
	idt_flush((uint32_t)&_idt_ptr);

	DEBUG(DL_DBG, ("GDT and TSS installed for core(%d) at 0x%p\n",
		       c->id, &_idt_ptr));
}

static void core_ctor(struct core *c, core_id_t id, int state)
{
	memset(c, 0, sizeof(struct core));
	
	LIST_INIT(&c->link);
	c->id = id;
	c->state = state;

	/* Initialize timer information */
	spinlock_init(&c->timer_lock, "tmr-lock");
	LIST_INIT(&c->timers);
	c->timer_enabled = FALSE;
}

void dump_core(struct core *c)
{
	kprintf("CORE(%d) detail information:\n", c->id);
	kprintf("vendor(%s)\n", c->arch.vendor_str);
	kprintf("core step(%d), phys_bits(%d), virt_bits(%d)\n",
		       c->arch.core_step, c->arch.max_phys_bits, c->arch.max_virt_bits);
	kprintf("core frequency(%lld), cycles per microseconds(%lld)\n",
		       c->arch.core_freq, c->arch.cycles_per_us);
	kprintf("sys_time_offset(%lld)\n\n", c->arch.sys_time_offset);
}

static void detect_core_features(struct core *c, struct core_features *f)
{
	uint32_t eax, ebx, ecx, edx;
	uint32_t *ptr;

	/* Get the highest supported standard level */
	x86_coreid(X86_COREID_VENDOR_ID, &f->highest_standard, &ebx, &ecx, &edx);
	if (f->highest_standard < X86_COREID_VENDOR_ID) return;

	/* Get standard feature information */
	x86_coreid(X86_COREID_FEATURE_INFO, &eax, &ebx, &f->standard_ecx, &f->standard_edx);

	/* Save model information */
	c->arch.core_step = eax & 0x0F;

	/* Get the highest supported extended level */
	x86_coreid(X86_COREID_EXT_MAX, &f->highest_extended, &ebx, &ecx, &edx);
	if (f->highest_extended & (1<<31)) {
		if (f->highest_extended >= X86_COREID_BRAND_STRING3) {
			/* Get vendor information */
			ptr = (uint32_t *)c->arch.vendor_str;
			x86_coreid(X86_COREID_BRAND_STRING1, &ptr[0], &ptr[1], &ptr[2], &ptr[3]);
			x86_coreid(X86_COREID_BRAND_STRING2, &ptr[4], &ptr[5], &ptr[6], &ptr[7]);
			x86_coreid(X86_COREID_BRAND_STRING3, &ptr[8], &ptr[9], &ptr[10], &ptr[11]);
		}

		if (f->highest_extended >= X86_COREID_ADDRESS_SIZE) {
			x86_coreid(X86_COREID_ADDRESS_SIZE, &eax, &ebx, &ecx, &edx);
			c->arch.max_phys_bits = eax & 0xFF;
			c->arch.max_virt_bits = (eax >> 8) & 0xFF;
		}
	} else {
		f->highest_extended = 0;
	}

	/* Specify default vendor name if it was not found */
	if (!c->arch.vendor_str[0]) {
		strcpy(c->arch.vendor_str, "Unknown vendor");
	}

	if (!c->arch.max_phys_bits) {
		c->arch.max_phys_bits = 32;
	}
	if (!c->arch.max_virt_bits) {
		c->arch.max_virt_bits = 48;
	}
}

static void arch_preinit_core()
{
	/* Initialize the global IDT and interrupt handler table */
	init_idt();

	/* Clear the irq handlers table and initialize them */
	init_IRQs();
}

static void arch_preinit_core_percore(struct core *c)
{
	struct core_features features;

	DEBUG(DL_DBG, ("arch_preinit_per_core\n"));

	/* If this is the boot CORE, a double fault stack will not have been
	 * allocated. Use the pre-allocated one in this case.
	 */
	if (c == &_boot_core) {
		c->arch.double_fault_stack = _boot_double_fault_stack;
	}

	/* Initialize and load descriptor tables */
	init_descriptor(c);

	/* Detect CORE features and information */
	detect_core_features(c, &features);

	/* If this is the boot CORE, copy features to the global features
	 * structure. Otherwise, check that the feature set matches the global
	 * features. We do not allow SMP configurations with different features
	 * on different COREs.
	 */
	if (c == &_boot_core) {
		memcpy(&_core_features, &features, sizeof(_core_features));

		/* Check the required features. Enable them when you need to use these
		 * features.
		 */
		if (features.highest_standard < X86_COREID_FEATURE_INFO) {
			PANIC("COREID feature information not supported");
		} else if (!_core_features.fpu || !_core_features.fxsr) {
			PANIC("CORE does not support FPU/FXSR");
		} else if (!_core_features.tsc) {
			PANIC("CORE does not support TSC");
		} else if (!_core_features.pge) {
			PANIC("CORE does not support PGE");
		}
	} else {
		if ((_core_features.highest_standard != features.highest_standard) ||
		    (_core_features.highest_extended != features.highest_extended) ||
		    (_core_features.standard_edx != features.standard_edx) ||
		    (_core_features.standard_ecx != features.standard_ecx) ||
		    (_core_features.extended_edx != features.extended_edx) ||
		    (_core_features.extended_ecx != features.extended_ecx)) {
			PANIC("CORE has different feature set to boot CORE");
		}
	}

	/* Get the CORE frequency */
	if (c == &_boot_core) {
		c->arch.core_freq = calculate_freq(calculate_core_freq);
		kprintf("core:%d core freq(%lld)\n", c->id, c->arch.core_freq);
	} else {
		c->arch.core_freq = _boot_core.arch.core_freq;
	}

	/* Work out the cycles per us */
	c->arch.cycles_per_us = c->arch.core_freq;
	do_div(c->arch.cycles_per_us, 1000000);
	DEBUG(DL_DBG, ("cycles per us(%d)\n", c->arch.cycles_per_us));
	if (c->arch.cycles_per_us == 0) {
		c->arch.cycles_per_us = 7;	// 7 is a value we get from test
	}

	/* Set NE/MP in CR0 (Numeric Error, Monitor Coprocessor) and clear EM (Emulation). */
	x86_write_cr0((x86_read_cr0() | X86_CR0_NE | X86_CR0_MP) & ~X86_CR0_EM);

	/* Configure the TSC offset for sys_time() */
	tsc_init_target();
}

static void arch_init_core_percore()
{
	init_lapic();
}

uint64_t calculate_core_freq()
{
	uint16_t shi, slo, ehi, elo, ticks;
	uint64_t start, end, cycles;

	/* Set the PIT to rate generator mode */
	outportb(0x43, 0x34);
	outportb(0x40, 0xFF);
	outportb(0x40, 0xFF);

	/* Wait for the cycle to begin */
	do {
		outportb(0x43, 0x00);
		slo = inportb(0x40);
		shi = inportb(0x40);
	} while (shi != 0xFF);

	/* Get the start TSC value */
	start = x86_rdtsc();

	/* Wait for the high byte to decrease to 128 */
	do {
		outportb(0x43, 0x00);
		elo = inportb(0x40);
		ehi = inportb(0x40);
	} while (ehi > 0x80);

	/* Get the end TSC value */
	end = x86_rdtsc();

	/* Calculate the difference between the values */
	cycles = end - start;
	ticks = ((ehi << 8) | elo) - ((shi << 8) | slo);

	/* Calculate frequency */
	ASSERT(PIT_BASE_FREQ > ticks);
	ASSERT(ticks != 0);

	return cycles * (PIT_BASE_FREQ / ticks);
}

uint64_t calculate_freq(uint64_t (*func)())
{
	size_t i;
	uint64_t ret = 0;
	uint64_t results[FREQ_ATTEMPTS];

	/* Get the frequencies */
	for (i = 0; i < FREQ_ATTEMPTS; i++) {
		results[i] = func();
	}

	for (i = 0; i < FREQ_ATTEMPTS; i++) {
		ret = ret > results[i] ? ret : results[i];
	}

	return ret;
}

struct core *core_register(core_id_t id, int state)
{
	size_t s;
	struct core *c;
	struct core **cores;

	c = kmalloc(sizeof(*c), 0);
	if (!c) {
		goto out;
	}
	
	core_ctor(c, id, state);

	if (id > _highest_core_id) {
		s = sizeof(struct core *) * (id + 1);
		cores = kmalloc(s, 0);
		if (!cores) {
			kfree(c);
			goto out;
		}
		
		memset(cores, 0, s);
		s = sizeof(struct core *) * (_highest_core_id + 1);
		memcpy(cores, _cores, s);
		
		_highest_core_id = id;
	}

	_cores[id] = c;
	_nr_cores++;

 out:
	return c;
}

void preinit_core_percore(struct core *c)
{
	arch_preinit_core_percore(c);

	/* Add the core to the running CORE list */
	list_add_tail(&CURR_CORE->link, &_running_cores);
}

void preinit_core()
{
	/* The boot CORE is initially assigned an ID of 0. It will be corrected
	 * once we have the ability to get the read ID.
	 */
	core_ctor(&_boot_core, 0, CORE_RUNNING);

	/* Perform architecture specific initialization. This initialize some
	 * state shared between all COREs
	 */
	arch_preinit_core();

	/* Initialize boot CORE */
	preinit_core_percore(&_boot_core);
}

void init_core_percore()
{
	arch_init_core_percore();
}

void init_core()
{
	/* Get the ID of the boot CORE */
	_boot_core.id = _highest_core_id = core_id();
	_nr_cores = 1;

	/* Create the initial CORE array and add the boot CORE to it */
	_cores = (struct core **)kmalloc((_highest_core_id+1) * sizeof(struct core *), 0);
	ASSERT(_cores != NULL);
	_cores[_boot_core.id] = &_boot_core;

	/* We are called on boot CORE */
	init_core_percore();
}
