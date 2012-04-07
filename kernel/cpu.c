#include <types.h>
#include <stddef.h>
#include <string.h>
#include "list.h"
#include "cpu.h"
#include "hal.h"
#include "mm/mmgr.h"
#include "exceptn.h"
#include "proc/task.h"
#include "matrix/debug.h"


/* Feature set present on all CPUs */
struct cpu_features _cpu_features;

/* Boot CPU structure */
struct cpu _boot_cpu;

/* Information about all CPUs */
static size_t _nr_cpus;
static size_t _highest_cpu_id;
struct list _running_cpus;
struct cpu **_cpus = NULL;

/* Double fault handler stack for the boot CPU */
static u_char _boot_double_fault_stack[KSTK_SIZE]__attribute__((aligned(PAGE_SIZE)));

extern struct irq_hook *_interrupt_handlers[];

static void cpu_ctor(struct cpu *c, cpu_id_t id, int state)
{
	memset(c, 0, sizeof(struct cpu));
	list_init(&c->header);
	c->id = id;
	c->state = state;
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

	/* Get the highest supported extended level */
	x86_cpuid(X86_CPUID_EXT_MAX, &f->highest_extended, &ebx, &ecx, &edx);
	if (f->highest_extended & (1<<31)) {
		if (f->highest_extended >= X86_CPUID_EXT_FEATURE) {
			/* Get the extended feature information */
			x86_cpuid(X86_CPUID_EXT_FEATURE, &eax, &ebx,
				  &f->extended_ecx, &f->extended_edx);
		}

		if (f->highest_extended >= X86_CPUID_ADDRESS_SIZE) {
			x86_cpuid(X86_CPUID_ADDRESS_SIZE, &eax, &ebx, &ecx, &edx);
			c->arch.max_phys_bits = eax & 0xFF;
			c->arch.max_virt_bits = (eax >> 8) & 0xFF;
		}
	} else {
		f->highest_extended = 0;
	}

	if (!c->arch.max_phys_bits)
		c->arch.max_phys_bits = 32;
	if (!c->arch.max_virt_bits)
		c->arch.max_virt_bits = 48;
}

static void arch_preinit_cpu()
{
	/* Install GDT and IDT, setup the exception handlers */
	init_gdt();
	init_idt();
	memset(&_interrupt_handlers[0], 0, sizeof(struct irq_hook *)*256);
	init_exception_handlers();
}

static void arch_preinit_per_cpu(struct cpu *c)
{
	struct cpu_features features;

	/* If this is the boot CPU, a double fault stack will not have been
	 * allocated. Use the pre-allocated one in this case.
	 */
	if (c == &_boot_cpu)
		c->arch.double_fault_stack = _boot_double_fault_stack;

	/* Initialize and load descriptor tables */
	;

	/* Detect CPU features and information */
	detect_cpu_features(c, &features);

	if (features.highest_standard < X86_CPUID_FEATURE_INFO)
		PANIC("CPUID feature information not supported");
	else if (!_cpu_features.fpu || !_cpu_features.fxsr)
		PANIC("CPU does not support FPU/FXSR");
	else if (!_cpu_features.tsc)
		PANIC("CPU does not support TSC");
	else if (!_cpu_features.pge)
		PANIC("CPU does not support PGE");

	/* Work out the cycles per us */
	c->arch.cycles_per_us = c->arch.cpu_freq;

	/* Enable PGE/OSFXSR */
	x86_write_cr4(x86_read_cr4() | X86_CR4_PGE | X86_CR4_OSFXSR);

	/* Enable NX/XD if supported */
	if (_cpu_features.xd)
		x86_write_msr(X86_MSR_EFER, x86_read_msr(X86_MSR_EFER) | X86_EFER_NXE);

	/* */
}

void preinit_cpu()
{
	/* The boot CPU is initially assigned an ID of 0. It will be corrected
	 * once we have the ability to get the real ID.
	 */
	cpu_ctor(&_boot_cpu, 0, CPU_RUNNING);

	/* Append it to the running CPU list */
	list_add_tail(&_running_cpus, &_boot_cpu.header);

	/* Perform architecture specific initialization. */
	arch_preinit_cpu();
}

void preinit_per_cpu(struct cpu *c)
{
	arch_preinit_per_cpu(c);
}

cpu_id_t cpu_id()
{
	return (cpu_id_t)lapic_id();
}

void init_cpu()
{
	/* Get the ID of the boot CPU. */
	_boot_cpu.id = _highest_cpu_id = cpu_id();
	_nr_cpus = 1;

	_cpus = kmalloc(sizeof(struct cpu *) * _highest_cpu_id + 1);
	_cpus[_boot_cpu.id] = &_boot_cpu;
}
