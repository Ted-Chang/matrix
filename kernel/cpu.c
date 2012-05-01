#include <types.h>
#include <stddef.h>
#include <string.h>
#include "list.h"
#include "cpu.h"
#include "hal/hal.h"
#include "mm/mlayout.h"
#include "mm/page.h"
#include "mm/kmem.h"
#include "exceptn.h"
#include "matrix/debug.h"
#include "hal/lapic.h"
#include "kd.h"			// Kernel Debugger
#include "pit.h"
#include "tsc.h"
#include "div64.h"


extern struct irq_hook *_irq_handlers[];
extern struct idt_ptr _idt_ptr;
extern void idt_flush();
extern void init_gdt();
extern void init_idt();
extern void init_tss(struct cpu *);

/* Feature set present on all CPUs */
struct cpu_features _cpu_features;

/* Boot CPU structure */
struct cpu _boot_cpu;

/* Information about all CPUs */
size_t _nr_cpus;
size_t _highest_cpu_id;
struct list _running_cpus;
struct cpu **_cpus = NULL;

/* Double fault handler stack for the boot CPU */
static u_char _boot_double_fault_stack[KSTK_SIZE]__attribute__((aligned(PAGE_SIZE)));

cpu_id_t cpu_id()
{
	return (cpu_id_t)lapic_id();
}

static kd_status_t kd_cmd_cpus(int argc, char **argv, struct kd_filter *filter)
{
	return KD_SUCCESS;
}

static void init_descriptor(struct cpu *c)
{
	/* Initialize and load GDT/TSS */
	init_gdt(c);
	init_tss(c);

	/* Point the CPU to the global IDT */
	idt_flush((uint32_t)&_idt_ptr);

	DEBUG(DL_DBG, ("GDT and TSS installed for cpu(%d) at 0x%p\n",
		       c->id, &_idt_ptr));
}

static void cpu_ctor(struct cpu *c, cpu_id_t id, int state)
{
	memset(c, 0, sizeof(struct cpu));
	LIST_INIT(&c->header);
	c->id = id;
	c->state = state;

	/* Initialize timer related stuff */
	LIST_INIT(&c->timers);
	spinlock_init(&c->timer_lock, "timer_lock");
}

/* Dump information of the specified CPU */
void dump_cpu(struct cpu *c)
{
	DEBUG(DL_DBG, ("CPU(%d) detail information:\n", c->id));
	DEBUG(DL_DBG, ("vendor: %s\n", c->arch.vendor_str));
	DEBUG(DL_DBG, ("cpu step(%d), phys_bits(%d), virt_bits(%d)\n",
		       c->arch.cpu_step, c->arch.max_phys_bits,
		       c->arch.max_virt_bits));
	DEBUG(DL_DBG, ("cycles per microseconds(%d), cpu frequency(%d)\n",
		       c->arch.cycles_per_us, c->arch.cpu_freq));
	if (lapic_enabled()) {
		DEBUG(DL_DBG, ("lapic frequency(%d), lapic timer conversion factor(%d)\n",
			       c->arch.lapic_freq, c->arch.lapic_timer_cv));
	}
}

/* Detect CPU features */
static void detect_cpu_features(struct cpu *c, struct cpu_features *f)
{
	uint32_t eax, ebx, ecx, edx;
	uint32_t *ptr;
	size_t i, j;
	char *str;

	/* Get the highest supported standard level */
	x86_cpuid(X86_CPUID_VENDOR_ID, &f->highest_standard, &ebx, &ecx, &edx);
	if (f->highest_standard < X86_CPUID_FEATURE_INFO)
		return;

	/* Get Standard feature information */
	x86_cpuid(X86_CPUID_FEATURE_INFO, &eax, &ebx, &f->standard_ecx, &f->standard_edx);

	/* Save model information */
	c->arch.cpu_step = eax & 0x0F;

	/* Get the highest supported extended level */
	x86_cpuid(X86_CPUID_EXT_MAX, &f->highest_extended, &ebx, &ecx, &edx);
	if (f->highest_extended & (1<<31)) {
		if (f->highest_extended >= X86_CPUID_EXT_FEATURE) {
			/* Get the extended feature information */
			x86_cpuid(X86_CPUID_EXT_FEATURE, &eax, &ebx,
				  &f->extended_ecx, &f->extended_edx);
		}

		if (f->highest_extended >= X86_CPUID_BRAND_STRING3) {
			/* Get vendor information */
			ptr = (uint32_t *)c->arch.vendor_str;
			x86_cpuid(X86_CPUID_BRAND_STRING1, &ptr[0], &ptr[1],
				  &ptr[2], &ptr[3]);
			x86_cpuid(X86_CPUID_BRAND_STRING2, &ptr[4], &ptr[5],
				  &ptr[6], &ptr[7]);
			x86_cpuid(X86_CPUID_BRAND_STRING3, &ptr[8], &ptr[9],
				  &ptr[10], &ptr[11]);
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
	if (!c->arch.vendor_str[0])
		strcpy(c->arch.vendor_str, "Unknown Vendor");

	if (!c->arch.max_phys_bits)
		c->arch.max_phys_bits = 32;
	if (!c->arch.max_virt_bits)
		c->arch.max_virt_bits = 48;
}

static void arch_preinit_cpu()
{
	/* Initialize the global IDT and the interrupt handler table */
	init_idt();
	init_exceptn_handlers();
}

uint64_t calculate_cpu_freq()
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
	} while(shi != 0xFF);

	/* Get the start TSC value */
	start = x86_rdtsc();

	/* Wait for the high byte to decrease to 128 */
	do {
		outportb(0x43, 0x00);
		elo = inportb(0x40);
		ehi = inportb(0x40);
	} while(ehi > 0x80);

	/* Get the end TSC value */
	end = x86_rdtsc();

	/* Calculate the differences between the values */
	cycles = end - start;
	ticks = ((ehi << 8) | elo) - ((shi << 8) | slo);

	/* Calculate frequency */
	ASSERT(PIT_BASE_FREQUENCY > ticks);
	return cycles * (PIT_BASE_FREQUENCY / ticks);
}

static void arch_preinit_per_cpu(struct cpu *c)
{
	struct cpu_features features;

	DEBUG(DL_DBG, ("arch_preinit_per_cpu\n"));
	
	/* If this is the boot CPU, a double fault stack will not have been
	 * allocated. Use the pre-allocated one in this case.
	 */
	if (c == &_boot_cpu)
		c->arch.double_fault_stack = _boot_double_fault_stack;

	/* Initialize and load descriptor tables */
	init_descriptor(c);

	/* Detect CPU features and information */
	detect_cpu_features(c, &features);

	/* If this is the boot CPU, copy features to the global features
	 * structure. Otherwise, check that the feature set matches the global
	 * features. We do not allow SMP configurations with different features
	 * on different CPUs.
	 */
	if (c == &_boot_cpu)
		memcpy(&_cpu_features, &features, sizeof(_cpu_features));
	else {
		if ((_cpu_features.highest_standard != features.highest_standard) ||
		    (_cpu_features.highest_extended != features.highest_extended) ||
		    (_cpu_features.standard_edx != features.standard_edx) ||
		    (_cpu_features.standard_ecx != features.standard_ecx) ||
		    (_cpu_features.extended_edx != features.extended_edx) ||
		    (_cpu_features.extended_ecx != features.extended_ecx))
			PANIC("CPU has different feature set to boot CPU");
	}

	/* Check for the required features. Enable them when you need to use these
	 * features.
	 */
	if (features.highest_standard < X86_CPUID_FEATURE_INFO)
		PANIC("CPUID feature information not supported");
	else if (!_cpu_features.fpu || !_cpu_features.fxsr)
		PANIC("CPU does not support FPU/FXSR");
	else if (!_cpu_features.tsc)
		PANIC("CPU does not support TSC");
	else if (!_cpu_features.pge)
		PANIC("CPU does not support PGE");

	/* Get the CPU frequency */
	if (c == &_boot_cpu)
		c->arch.cpu_freq = calculate_cpu_freq();
	else
		c->arch.cpu_freq = _boot_cpu.arch.cpu_freq;

	/* Work out the cycles per us */
	c->arch.cycles_per_us = ((uint32_t)c->arch.cpu_freq / 1000000);
	
	/* Initialize the TSC target */
	init_tsc_target();
}

/* Perform additional initialization of the current CPU */
static void arch_init_per_cpu()
{
	/* Register the Kernel Debugger command */
	kd_register_cmd("cpus", "Display a list of CPUs.", kd_cmd_cpus);

	/* Initialize the local advanced programmable interrupt controller */
	init_lapic();
}

void preinit_cpu()
{
	/* The boot CPU is initially assigned an ID of 0. It will be corrected
	 * once we have the ability to get the real ID.
	 */
	cpu_ctor(&_boot_cpu, 0, CPU_RUNNING);
	list_add_tail(&_boot_cpu.header, &_running_cpus);

	/* Perform architecture specific initialization. This initialize some
	 * state shared between all CPUs.
	 */
	arch_preinit_cpu();
}

void preinit_per_cpu(struct cpu *c)
{
	arch_preinit_per_cpu(c);
}

void init_per_cpu()
{
	arch_init_per_cpu();
}

void init_cpu()
{
	/* Get the ID of the boot CPU. */
	_boot_cpu.id = _highest_cpu_id = cpu_id();
	_nr_cpus = 1;

	/* Create the initial CPU array and the boot CPU to it */
	_cpus = (struct cpu **)kmem_alloc(sizeof(struct cpu *) * _highest_cpu_id + 1);
	_cpus[_boot_cpu.id] = &_boot_cpu;
}
