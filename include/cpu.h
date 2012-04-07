#ifndef __CPU_H__
#define __CPU_H__

#include "list.h"
#include "matrix/matrix.h"		// INLINE
#include "hal.h"			// Count of GDT and IDT

/* Flags in the CR0 Control Register */
#define X86_CR0_PE		(1<<0)	// Protect Mode Enable
#define X86_CR0_MP		(1<<1)	// Monitor Coprocessor
#define X86_CR0_EM		(1<<2)	// Emulation
#define X86_CR0_TS		(1<<3)	// Task Switched
#define X86_CR0_ET		(1<<4)	// Extension Type
#define X86_CR0_NE		(1<<5)	// Numeric Error
#define X86_CR0_WP		(1<<16)	// Write Protect
#define X86_CR0_AM		(1<<18)	// Alignment Mask
#define X86_CR0_NW		(1<<29)	// Not Write-through
#define X86_CR0_CD		(1<<30)	// Cache Disable
#define X86_CR0_PG		(1<<31)	// Paging Enable

/* Flags in the CR4 Control Register */
#define X86_CR4_VME		(1<<0)	// Virtual-8086 Mode Extensions
#define X86_CR4_PVI		(1<<1)	// Protected Mode Virtual Interrupts
#define X86_CR4_TSD		(1<<2)	// Time Stamp Disable
#define X86_CR4_DE		(1<<3)	// Debugging Extensions
#define X86_CR4_PSE		(1<<4)	// Page Size Extensions
#define X86_CR4_PAE		(1<<5)	// Physical Address Extensions
#define X86_CR4_MCE		(1<<6)	// Machine Check Enable
#define X86_CR4_PGE		(1<<7)	// Page Global Enable
#define X86_CR4_PCE		(1<<8)	// Performance-Monitoring Counter Enable
#define X86_CR4_OSFXSR		(1<<9)	// OS Support for FXSAVE/FSRSTOR
#define X86_CR4_OSXMMEXCPT	(1<<10)	// OS Support for Unmasked SIMD FPU Exceptions
#define X86_CR4_VMXE		(1<<13)	// VMX-Enable Bit
#define X86_CR4_SMXE		(1<<14)	// SMX-Enable Bit

/* Model Specific Register */
#define X86_MSR_TSC		0x10
#define X86_MSR_APIC_BASE	0x1B
#define X86_MSR_MTRR_BASE0	0x200
#define X86_MSR_MTRR_MASK0	0x201
#define X86_MSR_CR_PAT		0x277
#define X86_MSR_MTRR_DEF_TYPE	0x2FF
#define X86_MSR_EFER		0xC0000080
#define X86_MSR_STAR		0xC0000081
#define X86_MSR_LSTAR		0xC0000082
#define X86_MSR_FMASK		0xC0000084
#define X86_MSR_FS_BASE		0xC0000100
#define X86_MSR_GS_BASE		0xC0000101
#define X86_MSR_K_GS_BASE	0xC0000102

/* EFER MSR flags */
#define X86_EFER_SCE		(1<<0)	// System Call Enable
#define X86_EFER_LME		(1<<8)	// Long Mode (IA-32e) Enable
#define X86_EFER_LMA		(1<<10)	// Long Mode (IA-32e) Active
#define X86_EFER_NXE		(1<<11)	// Execute Disable (XD/NX) Bit Enable

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

	/* Standard CPUID Features (EDX) */
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

	/* Standard CPUID Features (ECX) */
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

	/* Extended CPUID Features (EDX) */
	union {
		struct {
			unsigned :11;
			unsigned syscall:1;
			unsigned :8;
			unsigned xd:1;
			unsigned :8;
			unsigned lmode:1;
		};
		uint32_t extended_edx;
	};

	/* Extended CPUID Features (ECX) */
	union {
		struct {
			unsigned lahf:1;
			unsigned :31;
		};
		uint32_t extended_ecx;
	};
};
typedef struct cpu_features cpu_features_t;

struct cpu;

/* Architecture specific CPU structure */
struct arch_cpu {
	struct cpu *parent;
	/* Timer conversion factors */
	uint64_t cycles_per_us;		// CPU cycles per us

	/* Per CPU structures */
	struct gdt gdt[NR_GDT_ENTRIES];	// Array of GDT descriptors
	struct tss tss;
	void *double_fault_stack;	// Pointer to the stack for double faults

	/* CPU information */
	uint64_t cpu_freq;		// CPU frequency in Hz
	uint64_t lapic_freq;		// LAPIC timer frequency in Hz

	uint8_t max_phys_bits;		// Maximum physical address bits
	uint8_t max_virt_bits;		// Maximum virtual address bits
};

/* CPU ID */
typedef uint32_t cpu_id_t;

struct cpu {
	struct list header;	// Link to running CPUs list
	
	cpu_id_t id;		// ID of this CPU
	struct arch_cpu arch;	// Architecture specific information
	
	/* Current state of the CPU */
	enum {
		CPU_OFFLINE,
		CPU_RUNNING,
	} state;
};
typedef struct cpu cpu_t;

extern struct cpu _boot_cpu;

/* Read CR4 register */
static INLINE uint32_t x86_read_cr4()
{
	uint32_t r;
	asm volatile("mov %%cr4, %0" : "=r"(r));
	return r;
}

/* Write CR4 register */
static INLINE void x86_write_cr4(uint32_t val)
{
	asm volatile("mov %0, %%cr4" :: "r"(val));
}

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

void preinit_cpu();
void preinit_per_cpu(struct cpu *c);
void init_cpu();

#endif	/* __CPU_H__ */
