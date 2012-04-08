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
#include "lapic.h"


extern struct irq_hook *_interrupt_handlers[];
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
static size_t _nr_cpus;
static size_t _highest_cpu_id;
struct list _running_cpus;
struct cpu **_cpus = NULL;

/* Double fault handler stack for the boot CPU */
static u_char _boot_double_fault_stack[KSTK_SIZE]__attribute__((aligned(PAGE_SIZE)));

cpu_id_t cpu_id()
{
	return (cpu_id_t)lapic_id();
}

static void init_descriptor(struct cpu *c)
{
	/* Initialize and load GDT/TSS */
	init_gdt(c);
	init_tss(c);

	/* Point the CPU to the global IDT */
	idt_flush((uint32_t)&_idt_ptr);

	DEBUG(DL_DBG, ("GDT and TSS installed for cpu:%d\n", c->id));
}

static void cpu_ctor(struct cpu *c, cpu_id_t id, int state)
{
	memset(c, 0, sizeof(struct cpu));
	list_init(&c->header);
	c->id = id;
	c->state = state;
}

/* Dump information of the specified CPU */
void dump_cpu(struct cpu *c)
{
	DEBUG(DL_DBG, ("cpu %d:\n"
		       "* phys_bits(%d), virt_bits(%d)\n",
		       c->id, c->arch.max_phys_bits, c->arch.max_virt_bits));
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
	/* Initialize the global IDT and the interrupt handler table */
	init_idt();
	memset(&_interrupt_handlers[0], 0, sizeof(struct irq_hook *)*256);
	init_exception_handlers();
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
}

void preinit_cpu()
{
	/* The boot CPU is initially assigned an ID of 0. It will be corrected
	 * once we have the ability to get the real ID.
	 */
	cpu_ctor(&_boot_cpu, 0, CPU_RUNNING);
	list_add_tail(&_running_cpus, &_boot_cpu.header);

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
	;
}

void init_cpu()
{
	/* Get the ID of the boot CPU. */
	_boot_cpu.id = _highest_cpu_id = cpu_id();
	_nr_cpus = 1;

	/* Create the initial CPU array and the boot CPU to it */
	_cpus = (struct cpu **)kmalloc(sizeof(struct cpu *) * _highest_cpu_id + 1);
	_cpus[_boot_cpu.id] = &_boot_cpu;
}
