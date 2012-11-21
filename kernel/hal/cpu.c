#include <types.h>
#include <string.h>
#include "hal/cpu.h"
#include "hal/hal.h"
#include "pit.h"
#include "matrix/debug.h"
#include "mm/mlayout.h"
#include "mm/page.h"
#include "div64.h"

extern struct idt_ptr _idt_ptr;
extern void init_idt();
extern void idt_flush();
extern void init_gdt(struct cpu *c);
extern void init_tss(struct cpu *c);

/* Feature set present on all CPUs */
struct cpu_features _cpu_features;

/* Boot CPU structure */
struct cpu _boot_cpu;

/* Information about all CPUs */
size_t _nr_cpus;
size_t _highest_cpu_id;

struct list _running_cpus = {
	.prev = &_running_cpus,
	.next = &_running_cpus,
};

/* Double fault handler stack for the boot CPU */
static u_char _boot_double_fault_stack[KSTACK_SIZE]__attribute__((aligned(PAGE_SIZE)));

cpu_id_t cpu_id()
{
	return 0;
}

static void init_descriptor(struct cpu *c)
{
	/* Initialize and load GDT/TSS */
	init_gdt(c);
	init_tss(c);

	/* Point this CPU to the global IDT */
	idt_flush((uint32_t)&_idt_ptr);

	DEBUG(DL_DBG, ("GDT and TSS installed for cpu(%d) at 0x%p\n",
		       c->id, &_idt_ptr));
}

static void cpu_ctor(struct cpu *c, cpu_id_t id, int state)
{
	memset(c, 0, sizeof(struct cpu));
	LIST_INIT(&c->link);
	c->id = id;
	c->state = state;

	/* Initialize timer information */
	LIST_INIT(&c->timers);
	c->timer_enabled = FALSE;
}

void dump_cpu(struct cpu *c)
{
	kprintf("CPU(%d) detail information:\n", c->id);
	kprintf("vendor(%s)\n", c->arch.vendor_str);
	kprintf("cpu step(%d), phys_bits(%d), virt_bits(%d)\n",
		       c->arch.cpu_step, c->arch.max_phys_bits, c->arch.max_virt_bits);
	kprintf("cpu frequency(%lld), cycles per microseconds(%lld)\n",
		       c->arch.cpu_freq, c->arch.cycles_per_us);
	kprintf("sys_time_offset(%lld)\n\n", c->arch.sys_time_offset);
}

static void detect_cpu_features(struct cpu *c, struct cpu_features *f)
{
	uint32_t eax, ebx, ecx, edx;
	uint32_t *ptr;

	/* Get the highest supported standard level */
	x86_cpuid(X86_CPUID_VENDOR_ID, &f->highest_standard, &ebx, &ecx, &edx);
	if (f->highest_standard < X86_CPUID_VENDOR_ID) return;

	/* Get standard feature information */
	x86_cpuid(X86_CPUID_FEATURE_INFO, &eax, &ebx, &f->standard_ecx, &f->standard_edx);

	/* Save model information */
	c->arch.cpu_step = eax & 0x0F;

	/* Get the highest supported extended level */
	x86_cpuid(X86_CPUID_EXT_MAX, &f->highest_extended, &ebx, &ecx, &edx);
	if (f->highest_extended & (1<<31)) {
		if (f->highest_extended >= X86_CPUID_BRAND_STRING3) {
			/* Get vendor information */
			ptr = (uint32_t *)c->arch.vendor_str;
			x86_cpuid(X86_CPUID_BRAND_STRING1, &ptr[0], &ptr[1], &ptr[2], &ptr[3]);
			x86_cpuid(X86_CPUID_BRAND_STRING2, &ptr[4], &ptr[5], &ptr[6], &ptr[7]);
			x86_cpuid(X86_CPUID_BRAND_STRING3, &ptr[8], &ptr[9], &ptr[10], &ptr[11]);
		}

		if (f->highest_extended >= X86_CPUID_ADDRESS_SIZE) {
			x86_cpuid(X86_CPUID_ADDRESS_SIZE, &eax, &ebx, &ecx, &edx);
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

static uint64_t calculate_cpu_freq()
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

	return cycles * (PIT_BASE_FREQ / ticks);
}

static void arch_preinit_cpu()
{
	/* Initialize the global IDT and interrupt handler table */
	init_idt();

	/* Clear the irq handlers table and initialize them */
	init_irqs();
}

static void arch_preinit_cpu_percpu(struct cpu *c)
{
	struct cpu_features features;

	DEBUG(DL_DBG, ("arch_preinit_per_cpu\n"));

	/* If this is the boot CPU, a double fault stack will not have been
	 * allocated. Use the pre-allocated one in this case.
	 */
	if (c == &_boot_cpu) {
		c->arch.double_fault_stack = _boot_double_fault_stack;
	}

	/* Initialize and load descriptor tables */
	init_descriptor(c);

	/* Detect CPU features and information */
	detect_cpu_features(c, &features);

	/* If this is the boot CPU, copy features to the global features
	 * structure. Otherwise, check that the feature set matches the global
	 * features. We do not allow SMP configurations with different features
	 * on different CPUs.
	 */
	if (c == &_boot_cpu) {
		memcpy(&_cpu_features, &features, sizeof(_cpu_features));
	} else {
		if ((_cpu_features.highest_standard != features.highest_standard) ||
		    (_cpu_features.highest_extended != features.highest_extended) ||
		    (_cpu_features.standard_edx != features.standard_edx) ||
		    (_cpu_features.standard_ecx != features.standard_ecx) ||
		    (_cpu_features.extended_edx != features.extended_edx) ||
		    (_cpu_features.extended_ecx != features.extended_ecx)) {
			PANIC("CPU has different feature set to boot CPU");
		}
	}

	/* Check for the required features. Enable them when you need to use these
	 * features.
	 */
	if (features.highest_standard < X86_CPUID_FEATURE_INFO) {
		PANIC("CPUID feature information not supported");
	} else if (!_cpu_features.fpu || !_cpu_features.fxsr) {
		PANIC("CPU does not support FPU/FXSR");
	} else if (!_cpu_features.tsc) {
		PANIC("CPU does not support TSC");
	} else if (!_cpu_features.pge) {
		PANIC("CPU does not support PGE");
	}

	/* Get the CPU frequency */
	if (c == &_boot_cpu) {
		c->arch.cpu_freq = calculate_cpu_freq();
	} else {
		c->arch.cpu_freq = _boot_cpu.arch.cpu_freq;
	}

	/* Work out the cycles per us */
	c->arch.cycles_per_us = c->arch.cpu_freq;
	do_div(c->arch.cycles_per_us, 1000000);
	ASSERT(c->arch.cycles_per_us != 0);

	/* Configure the TSC offset for sys_time() */
	tsc_init_target();
}

static void arch_init_cpu_percpu()
{
	/* TODO: Initialize LAPIC */
	;
}

void preinit_cpu_percpu(struct cpu *c)
{
	arch_preinit_cpu_percpu(c);
}

void preinit_cpu()
{
	/* The boot CPU is initially assigned an ID of 0. It will be corrected
	 * once we have the ability to get the read ID.
	 */
	cpu_ctor(&_boot_cpu, 0, CPU_RUNNING);
	list_add_tail(&_boot_cpu.link, &_running_cpus);

	/* Perform architecture specific initialization. This initialize some
	 * state shared between all CPUs
	 */
	arch_preinit_cpu();
}

void init_cpu_percpu()
{
	arch_init_cpu_percpu();
}

void init_cpu()
{
	/* Get the ID of the boot CPU */
	_boot_cpu.id = _highest_cpu_id = cpu_id();
	_nr_cpus = 1;
}
