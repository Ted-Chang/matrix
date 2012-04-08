#ifndef __LAPIC_H__
#define __LAPIC_H__

#define LAPIC_REG_APIC_ID	8
#define LAPIC_REG_APIC_VERSION	12
#define LAPIC_REG_TPR		32
#define LAPIC_REG_APR		36
#define LAPIC_REG_PPR		40
#define LAPIC_REG_EOI		44

void init_lapic();
uint32_t lapic_id();

#endif	/* __LAPIC_H__ */
