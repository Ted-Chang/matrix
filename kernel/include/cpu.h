#ifndef __CPU_H__
#define __CPU_H__

#include "matrix/matrix.h"	// For INLINE
#include "list.h"

/* Standard CPUID function definitions */
#define X86_CPUID_VENDOR_ID	0x00000000
#define X86_CPUID_FEATURE_INFO	0x00000001

/* Extended CPUID function definitions */
#define X86_CPUID_EXT_MAX	0x80000000
#define X86_CPUID_EXT_FEATURE	0x80000001
#define X86_CPUID_BRAND_STRING1	0x80000002
#define X86_CPUID_BRAND_STRING2	0x80000003
#define X86_CPUID_BRAND_STRING3	0x80000004
#define X86_CPUID_L2_CACHE	0x80000006
#define X86_CPUID_ADVANCED_PM	0x80000007
#define X86_CPUID_ADDRESS_SIZE	0x80000008

struct cpu_features {
	uint32_t highest_standard;	// Highest standard function
	uint32_t highest_extended;	// Highest extended function

	/* Standard CPUID features (EDX) */
	union {
		struct {
			unsigned fpu:1;
			unsigned vme:1;
			unsigned de:1;
			unsigned pse:1;
			unsigned tsc:1;
			unsigned msr:1;
			unsigned pae:1;
			unsigned mce:1;
			unsigned cx8:1;
			unsigned apic:1;
		};
		uint32_t standard_edx;
	};

	/* Standard CPUID features (ECX) */
	union {
		struct {
			unsigned sse3:1;
			unsigned pclmulqdq:1;
			unsigned dtes64:1;
			unsigned monitor:1;
			unsigned dscpl:1;
			unsigned vmx:1;
			unsigned smx:1;
			unsigned est:1;
			unsigned tm2:1;
		};
		uint32_t standard_ecx;
	};
};
typedef struct cpu_features cpu_features_t;

struct cpu;

/* Architecture specific CPU structure */
struct arch_cpu {
	struct cpu *parent;

	/* Per CPU structures */
	void *double_fault_stack;	// Pointer to the stack for double faults

	/* CPU information */
	uint32_t cpu_freq;		// CPU frequency in Hz
	char vendor_str[64];		// Vendor string
	uint8_t cpu_step;		// CPU step
	uint8_t max_phys_bits;		// Maximum physical address bits
	uint8_t max_virt_bits;		// Maximum virtual address bits
};

/* CPU ID */
typedef uint32_t cpu_id_t;

struct cpu {
	struct list link;		// Link to running CPUs list

	cpu_id_t id;			// ID of this CPU
	struct arch_cpu arch;		// Architecture specific information
	
	/* Current state of the CPU */
	enum {
		CPU_OFFLINE,
		CPU_RUNNING,
	} state;
};
typedef struct cpu cpu_t;

extern struct cpu _boot_cpu;

/* Execute the CPUID instruction */
static INLINE void x86_cpuid(uint32_t level, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
	asm volatile("cpuid" : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d) : "0"(level));
}

/* Invalidate a TLB entry */
static INLINE void x86_invlpg(uint32_t addr)
{
	asm volatile("invlpg (%0)" :: "r"(addr));
}

/* Halt the CPU */
static INLINE void cpu_halt()
{
	while (TRUE) {
		asm volatile("cli;hlt");
	}
}

/* Yield the CPU */
static INLINE void cpu_idle()
{
	asm volatile("sti;hlt;cli");
}


void init_cpu();

#endif	/* __CPU_H__ */
