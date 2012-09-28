#ifndef __CPU_H__
#define __CPU_H__

#include "matrix/matrix.h"	// For INLINE
#include "list.h"

/* Model Specific Register */
#define X86_MSR_TSC		0x10		// Time Stamp Counter (TSC)
#define X86_MSR_APIC_BASE	0x1B		// LAPIC base address

/* Standard CPUID function definitions */
#define X86_CPUID_VENDOR_ID	0x00000000
#define X86_CPUID_FEATURE_INFO	0x00000001
#define X86_CPUID_CACHE_DESC	0x00000002
#define X86_CPUID_SERIAL_NUM	0x00000003
#define X86_CPUID_CACHE_PARMS	0x00000004
#define X86_CPUID_MONITOR_MWAIT	0x00000005
#define X86_CPUID_DTS_POWER	0x00000006
#define X86_CPUID_DCA		0x00000009
#define X86_CPUID_PERFMON	0x0000000A
#define X86_CPUID_X2APIC	0x0000000B
#define X86_CPUID_XSAVE		0x0000000D

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
			unsigned :1;
			unsigned sep:1;
			unsigned mtrr:1;
			unsigned pge:1;
			unsigned mca:1;
			unsigned cmov:1;
			unsigned pat:1;
			unsigned pse36:1;
			unsigned psn:1;
			unsigned clfsh:1;
			unsigned :1;
			unsigned ds:1;
			unsigned acpi:1;
			unsigned mmx:1;
			unsigned fxsr:1;
			unsigned sse:1;
			unsigned sse2:1;
			unsigned ss:1;
			unsigned htt:1;
			unsigned tm:1;
			unsigned :1;
			unsigned pbe:1;
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
			unsigned ssse3:1;
			unsigned cnxtid:1;
			unsigned :2;
			unsigned fma:1;
			unsigned cmpxchg16b:1;
			unsigned xtpr:1;
			unsigned pdcm:1;
			unsigned :2;
			unsigned pcid:1;
			unsigned dca:1;
			unsigned sse4_1:1;
			unsigned sse4_2:1;
			unsigned x2apic:1;
			unsigned movbe:1;
			unsigned popcnt:1;
			unsigned tscd:1;
			unsigned aes:1;
			unsigned xsave:1;
			unsigned osxsave:1;
			unsigned avx:1;
			unsigned :3;
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
	uint64_t cpu_freq;		// CPU frequency in Hz
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

/* Read an MSR */
static INLINE uint64_t x86_read_msr(uint32_t msr)
{
	uint32_t low, high;

	asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
	return (((uint64_t)high) << 32) | low;
}

/* Write an MSR */
static INLINE void x86_write_msr(uint32_t msr, uint64_t value)
{
	asm volatile("wrmsr" :: "a"((uint32_t)value), "d"((uint32_t)(value >> 32)), "c"(msr));
}

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

/* Read the Time Stamp Counter */
static INLINE uint64_t x86_rdtsc()
{
	uint32_t high, low;

	asm volatile("rdtsc" : "=a"(low), "=d"(high));
	return ((uint64_t)high << 32) | low;
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
