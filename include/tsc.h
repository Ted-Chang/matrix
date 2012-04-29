#ifndef __TSC_H__
#define __TSC_H__

/* Read the Time Stamp Counter */
static INLINE uint64_t x86_rdtsc()
{
	uint32_t high, low;
	asm volatile("rdtsc" : "=a"(low), "=d"(high));
	return ((uint64_t)high << 32) | low;
}

extern void init_tsc_target();

#endif	/* __TSC_H__ */
