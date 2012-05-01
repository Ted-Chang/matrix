#ifndef __CPU_H__
#define __CPU_H__

#include "list.h"
#include "matrix/matrix.h"		// INLINE
#include "hal/hal.h"			// Count of GDT and IDT
#include "hal/spinlock.h"

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

/* Flags in the debug status register (DR6) */
#define X86_DR6_B0		(1<<0)	// Breakpoint 0 condition detected
#define X86_DR6_B1		(1<<1)	// Breakpoint 1 condition detected
#define X86_DR6_B2		(1<<2)	// Breakpoint 2 condition detected
#define X86_DR6_B3		(1<<3)	// Breakpoint 3 condition detected
#define X86_DR6_BD		(1<<13)	// Debug register access
#define X86_DR6_BS		(1<<14)	// Single-stepped
#define X86_DR6_BT		(1<<15)	// Task Switch

/* Flags in the debug control register (DR7) */
#define X86_DR7_G0		(1<<1)	// Global breakpoint 0 enable
#define X86_DR7_G1		(1<<3)	// Global breakpoint 1 enable
#define X86_DR7_G2		(1<<5)	// Global breakpoint 2 enable
#define X86_DR7_G3		(1<<7)	// Global breakpoint 3 enable

/* Definitions for bits in the EFLAGS/RFLAGS register */
#define X86_FLAGS_CF		(1<<0)	// Carry Flag
#define X86_FLAGS_ALWAYS1	(1<<1)	// Flag that must always be 1
#define X86_FLAGS_PF		(1<<2)	// Parity Flag
#define X86_FLAGS_AF		(1<<4)	// Auxilary Carry Flag
#define X86_FLAGS_ZF		(1<<6)	// Zero Flag
#define X86_FLAGS_SF		(1<<7)	// Sign Flag
#define X86_FLAGS_TF		(1<<8)	// Trap Flag
#define X86_FLAGS_IF		(1<<9)	// Interrupt enable Flag
#define X86_FLAGS_DF		(1<<10)	// Direction Flag
#define X86_FLAGS_OF		(1<<11)	// Overflow Flag
#define X86_FLAGS_NT		(1<<14)	// Nested Task Flag
#define X86_FLAGS_RF		(1<<16)	// Resume Flag
#define X86_FLAGS_VM		(1<<17)	// Virtual-8086 Mode
#define X86_FLAGS_AC		(1<<18)	// Alignment Check
#define X86_FLAGS_VIF		(1<<19)	// Virtual Interrupt Flag
#define X86_FLAGS_VIP		(1<<20)	// Virtual Interrupt Pending Flag
#define X86_FLAGS_ID		(1<<21)	// ID Flag

/* Model Specific Register */
#define X86_MSR_TSC		0x10	// Time Stamp Counter (TSC)
#define X86_MSR_APIC_BASE	0x1B	// LAPIC base address
#define X86_MSR_MTRR_BASE0	0x200	// Base of the variable length MTRR base registers
#define X86_MSR_MTRR_MASK0	0x201	// Base of the variable length MTRR mask registers
#define X86_MSR_CR_PAT		0x277	// PAT
#define X86_MSR_MTRR_DEF_TYPE	0x2FF	// Default MTRR type
#define X86_MSR_EFER		0xC0000080 // Extended Feature Enable register
#define X86_MSR_STAR		0xC0000081 // System Call Target Address
#define X86_MSR_LSTAR		0xC0000082 // 64-bit System Call Target Address
#define X86_MSR_FMASK		0xC0000084 // System Call Flag Mask
#define X86_MSR_FS_BASE		0xC0000100 // FS segment base register
#define X86_MSR_GS_BASE		0xC0000101 // GS segment base register
#define X86_MSR_K_GS_BASE	0xC0000102 // GS base to switch to with SWAPGS

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

extern struct cpu_features _cpu_features;

struct va_space;
struct cpu;

/* Architecture specific CPU structure */
struct arch_cpu {
	struct cpu *parent;

	/* Per CPU structures */
	struct gdt gdt[NR_GDT_ENTRIES];	// Array of GDT descriptors
	struct tss tss;			// Task State Segment
	void *double_fault_stack;	// Pointer to the stack for double faults

	/* Time conversion factors */
	uint32_t cycles_per_us;		// CPU cycles per us
	uint32_t lapic_timer_cv;	// LAPIC timer conversion factor
	useconds_t sys_time_offset;	// Value to subtract from TSC value for sys_time()

	/* CPU information */
	uint32_t cpu_freq;		// CPU frequency in Hz
	uint32_t lapic_freq;		// LAPIC timer frequency in Hz
	char vendor_str[64];		// Vendor string
	uint8_t cpu_step;		// CPU Step
	uint8_t max_phys_bits;		// Maximum physical address bits
	uint8_t max_virt_bits;		// Maximum virtual address bits
};

/* CPU ID */
typedef uint16_t cpu_id_t;
struct cpu {
	struct list header;		// Link to running CPUs list
	
	cpu_id_t id;			// ID of this CPU
	struct arch_cpu arch;		// Architecture specific information
	
	/* Current state of the CPU */
	enum {
		CPU_OFFLINE,
		CPU_RUNNING,
	} state;

	struct va_space *space;		// Address space currently in use

	/* Timer related structures */
	struct list timers;		// List of active timers
	struct spinlock timer_lock;	// Lock to protect timer list
	boolean_t timer_enabled;	// Whether the timer for this CPU is enabled
};
typedef struct cpu cpu_t;

#define CURR_CPU	((struct cpu *)cpu_get_pointer())

extern struct cpu _boot_cpu;
extern size_t _highest_cpu_id;
extern size_t _nr_cpus;
extern struct list _running_cpus;
extern struct cpu **_cpus;

/* Read CR0 register */
static INLINE uint32_t x86_read_cr0()
{
	uint32_t r;
	asm volatile("mov %%cr0, %0" : "=r"(r));
	return r;
}

/* Write CR0 register */
static INLINE void x86_write_cr0(uint32_t val)
{
	asm volatile("mov %0, %%cr4" :: "r"(val));
}

/* Read CR3 register */
static INLINE uint32_t x86_read_cr3()
{
	uint32_t r;
	asm volatile("mov %%cr3, %0" : "=r"(r));
	return r;
}

/* Write CR3 register */
static INLINE void x86_write_cr3(uint32_t val)
{
	asm volatile("mov %0, %%cr3" :: "r"(val));
}

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

/* Invalidate a TLB entry */
static INLINE void x86_invlpg(uint32_t addr)
{
	asm volatile("invlpg (%0)" :: "r"(addr));
}

/* Get the current CPU pointer, the point was set when we initialize
 * the GDT for the CPU
 */
static INLINE struct cpu *cpu_get_pointer()
{
	uint32_t addr;
	//asm ("mov %%gs:0, %0" : "=r"(addr));
	addr = (uint32_t)x86_read_msr(X86_MSR_GS_BASE);
	return (struct cpu *)addr;
}

static INLINE void cpu_halt()
{
	while (TRUE)
		asm volatile("cli;hlt");
}

static INLINE void cpu_idle()
{
	asm volatile("sti;hlt;cli");
}

extern cpu_id_t cpu_id();
extern void dump_cpu(struct cpu *c);

extern void preinit_cpu();
extern void preinit_per_cpu(struct cpu *c);
extern void init_per_cpu();
extern void init_cpu();

#endif	/* __CPU_H__ */
