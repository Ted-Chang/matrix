#include <types.h>
#include <string.h>
#include "cpu.h"
#include "pit.h"
#include "hal.h"
#include "matrix/debug.h"

/* Boot CPU structure */
struct cpu _boot_cpu;

size_t _nr_cpus;
size_t _highest_cpu_id;

static void cpu_ctor(struct cpu *c, cpu_id_t id, int state)
{
	memset(c, 0, sizeof(struct cpu));
	LIST_INIT(&c->link);
	c->id = id;
	c->state = state;

	/* Initialize timer information */
	LIST_INIT(&c->timers);
}

void dump_cpu(struct cpu *c)
{
	DEBUG(DL_DBG, ("CPU(%d) detail information:\n", c->id));
	DEBUG(DL_DBG, ("vendor: %s\n", c->arch.vendor_str));
	DEBUG(DL_DBG, ("cpu step(%d), phys_bits(%d), virt_bits(%d)\n",
		       c->arch.cpu_step, c->arch.max_phys_bits, c->arch.max_virt_bits));
	DEBUG(DL_DBG, ("cpu frequency(%16d)\n", c->arch.cpu_freq));
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
	if (!c->arch.vendor_str[0])
		strcpy(c->arch.vendor_str, "Unknown vendor");

	if (!c->arch.max_phys_bits)
		c->arch.max_phys_bits = 32;
	if (!c->arch.max_virt_bits)
		c->arch.max_virt_bits = 48;

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

cpu_id_t cpu_id()
{
	return 0;
}

void init_cpu()
{
	struct cpu_features features;
	
	/* The boot CPU is initially assigned an ID of 0. It will be corrected
	 * once we have the ability to get the read ID.
	 */
	cpu_ctor(&_boot_cpu, 0, CPU_RUNNING);

	/* Detect CPU feature and information. */
	detect_cpu_features(&_boot_cpu, &features);

	/* Get the CPU frequency */
	_boot_cpu.arch.cpu_freq = calculate_cpu_freq();
	
	_boot_cpu.id = _highest_cpu_id = cpu_id();
	_nr_cpus = 1;

	dump_cpu(&_boot_cpu);
}
