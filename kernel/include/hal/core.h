#ifndef __CORE_H__
#define __CORE_H__

#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"	// For INLINE
#include "list.h"
#include "hal/hal.h"

/* Model Specific Register */
#define X86_MSR_TSC		0x10		// Time Stamp Counter (TSC)
#define X86_MSR_APIC_BASE	0x1B		// LAPIC base address
#define X86_MSR_MTRR_BASE0	0x200		// Base of the variable length MTRR base register
#define X86_MSR_MTRR_MASK0	0x201		// Base of the variable length MTRR mask register
#define X86_MSR_CR_PAT		0x277		// PAT
#define X86_MSR_MTRR_DEF_TYPE	0x2FF		// Default MTRR type
#define X86_MSR_EFER		0xC0000080	// Extended Feature Enable register
#define X86_MSR_STAR		0xC0000081	// System Call Target Address
#define X86_MSR_LSTAR		0xC0000082	// 64-bit System Call Target Address
#define X86_MSR_FMASK		0xC0000084	// System Call Flag Mask
#define X86_MSR_FS_BASE		0xC0000100	// FS segment base register
#define X86_MSR_GS_BASE		0xC0000101	// GS segment base register
#define X86_MSR_K_GS_BASE	0xC0000102	// GS base switch to with SWAPGS

/* Standard COREID function definitions */
#define X86_COREID_VENDOR_ID	0x00000000
#define X86_COREID_FEATURE_INFO	0x00000001
#define X86_COREID_CACHE_DESC	0x00000002
#define X86_COREID_SERIAL_NUM	0x00000003
#define X86_COREID_CACHE_PARMS	0x00000004
#define X86_COREID_MONITOR_MWAIT 0x00000005
#define X86_COREID_DTS_POWER	0x00000006
#define X86_COREID_DCA		0x00000009
#define X86_COREID_PERFMON	0x0000000A
#define X86_COREID_X2APIC	0x0000000B
#define X86_COREID_XSAVE	0x0000000D

/* Extended COREID function definitions */
#define X86_COREID_EXT_MAX	0x80000000
#define X86_COREID_EXT_FEATURE	0x80000001
#define X86_COREID_BRAND_STRING1 0x80000002
#define X86_COREID_BRAND_STRING2 0x80000003
#define X86_COREID_BRAND_STRING3 0x80000004
#define X86_COREID_L2_CACHE	0x80000006
#define X86_COREID_ADVANCED_PM	0x80000007
#define X86_COREID_ADDRESS_SIZE	0x80000008

/* Features the CORE supported */
struct core_features {
	uint32_t highest_standard;		// Highest standard function
	uint32_t highest_extended;		// Highest extended function

	/* Standard COREID features (EDX) */
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

	/* Standard COREID features (ECX) */
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

	/* Extended COREID Features (EDX) */
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

	/* Extended COREID Features (ECX) */
	union {
		struct {
			unsigned lahf:1;
			unsigned :31;
		};
		uint32_t extended_ecx;
	};
};
typedef struct core_features core_features_t;

struct core;

/* Architecture specific CORE structure */
struct arch_core {
	struct core *parent;

	/* Per CORE structures */
	struct gdt gdt[NR_GDT_ENTRIES];	// Array of GDT descriptors
	struct tss tss;			// Task State Segment
	void *double_fault_stack;	// Pointer to the stack for double faults

	/* Time conversion factors */
	uint64_t cycles_per_us;		// CORE cycles per us
	int64_t sys_time_offset;	// Value to subtract from TSC value for sys_time()
	
	/* CORE information */
	uint64_t core_freq;		// CORE frequency in Hz
	char vendor_str[64];		// Vendor string
	uint8_t core_step;		// CORE step
	uint8_t max_phys_bits;		// Maximum physical address bits
	uint8_t max_virt_bits;		// Maximum virtual address bits
};

/* CORE ID */
typedef uint32_t core_id_t;

struct core {
	struct list link;		// Link to running COREs list

	core_id_t id;			// ID of this CORE
	struct arch_core arch;		// Architecture specific information
	
	/* Current state of the CORE */
	enum {
		CORE_OFFLINE,
		CORE_RUNNING,
	} state;

	/* Scheduler information */
	struct sched_core *sched;	// Scheduler run queues/timers
	struct thread *thread;		// Currently executing thread
	struct mmu_ctx *aspace;		// Address space currently in use
	struct list timers;		// List of active timers
	boolean_t timer_enabled;	// Whether timer is enabled on this CORE
};
typedef struct core core_t;

/* Expands to a pointer to the CORE structure of the current CORE */
#define CURR_CORE	((struct core *)core_get_pointer())

extern struct core _boot_core;
extern size_t _nr_cores;
extern size_t _highest_core_id;
extern struct list _running_cores;

/* Read DR0 */
static INLINE uint32_t x86_read_dr0()
{
	uint32_t r;

	asm volatile("mov %%dr0, %0" : "=r"(r));
	return r;
}

/* Write DR0 */
static INLINE void x86_write_dr0(uint32_t val)
{
	asm volatile("mov %0, %%dr0" :: "r"(val));
}

/* Read DR1 */
static INLINE uint32_t x86_read_dr1()
{
	uint32_t r;

	asm volatile("mov %%dr1, %0" : "=r"(r));
	return r;
}

/* Write DR1 */
static INLINE void x86_write_dr1(uint32_t val)
{
	asm volatile("mov %0, %%dr1" :: "r"(val));
}

/* Read DR2 */
static INLINE uint32_t x86_read_dr2()
{
	uint32_t r;

	asm volatile("mov %%dr2, %0" : "=r"(r));
	return r;
}

/* Write DR2 */
static INLINE void x86_write_dr2(uint32_t val)
{
	asm volatile("mov %0, %%dr2" :: "r"(val));
}

/* Read DR3 */
static INLINE uint32_t x86_read_dr3()
{
	uint32_t r;

	asm volatile("mov %%dr3, %0" : "=r"(r));
	return r;
}

/* Write DR3 */
static INLINE void x86_write_dr3(uint32_t val)
{
	asm volatile("mov %0, %%dr3" :: "r"(val));
}

/* Read DR6 */
static INLINE uint32_t x86_read_dr6()
{
	uint32_t r;

	asm volatile("mov %%dr6, %0" : "=r"(r));
	return r;
}

/* Write DR6 */
static INLINE void x86_write_dr6(uint32_t val)
{
	asm volatile("mov %0, %%dr6" :: "r"(val));
}

/* Read DR7 */
static INLINE uint32_t x86_read_dr7()
{
	uint32_t r;

	asm volatile("mov %%dr7, %0" : "=r"(r));
	return r;
}

/* Write DR7 */
static INLINE void x86_write_dr7(uint32_t val)
{
	asm volatile("mov %0, %%dr7" :: "r"(val));
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

/* Execute the COREID instruction */
static INLINE void x86_coreid(uint32_t level, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
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

/* Get the current CORE pointer, the pointer was set when we initialize
 * the GDT for the CORE
 */
static INLINE struct core *core_get_pointer()
{
	uint32_t addr;
	
	//addr = (uint32_t)x86_read_msr(X86_MSR_GS_BASE);
	addr = (uint32_t)&_boot_core;
	return (struct core *)addr;
}

/* Halt the CORE */
static INLINE void core_halt()
{
	while (TRUE) {
		asm volatile("cli;hlt");
	}
}

/* Yield the CORE */
static INLINE void core_idle()
{
	asm volatile("sti;hlt;cli");
}

/* CORE-specific spin loop hint */
static INLINE void core_spin_hint()
{
	asm volatile("pause");
}

extern void dump_core(struct core *c);
extern core_id_t core_id();
extern void preinit_core_percore();
extern void preinit_core();
extern void init_core_percore();
extern void init_core();

#endif	/* __CORE_H__ */
