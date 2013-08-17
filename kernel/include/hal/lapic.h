#ifndef __LAPIC_H__
#define __LAPIC_H__

/* Local APIC register offsets */
#define LAPIC_REG_APIC_ID		8	// Local APIC ID
#define LAPIC_REG_APIC_VERSION		12	// Local APIC Version
#define LAPIC_REG_TPR			32	// Task Priority Register (TPR)
#define LAPIC_REG_APR			36	// Arbitration Priority Register (APR)
#define LAPIC_REG_PPR			40	// Processor Priority Register (PPR)
#define LAPIC_REG_EOI			44	// EOI
#define LAPIC_REG_LVT_TIMER		200	// LVT Timer
#define LAPIC_REG_SPURIOUS		60	// Spurious Interrupt Vector
#define LAPIC_REG_TIMER_INITIAL		224	// Timer Initial Count
#define LAPIC_REG_TIMER_CURRENT		228	// Timer Current Count
#define LAPIC_REG_TIMER_DIVIDER		248	// Timer Divide Configuration

/* Local APIC Divide Configuration Register values */
#define LAPIC_TIMER_DIV1		0xB	// Divide by 1
#define LAPIC_TIMER_DIV2		0x0	// Divide by 2
#define LAPIC_TIMER_DIV4		0x1	// Divide by 4
#define LAPIC_TIMER_DIV8		0x2	// Divide by 8
#define LAPIC_TIMER_DIV16		0x3	// Divide by 16
#define LAPIC_TIMER_DIV32		0x8	// Divide by 32
#define LAPIC_TIMER_DIV64		0x9	// Divide by 64
#define LAPIC_TIMER_DIV128		0xA	// Divide by 128

/* Local APIC interrupt handlers */
#define LAPIC_VECT_TIMER		0xF0
#define LAPIC_VECT_SPURIOUS		0xF1
#define LAPIC_VECT_IPI			0xF2

extern boolean_t lapic_enabled();
extern uint32_t lapic_id();
extern void init_lapic();

#endif	/* __LAPIC_H__ */
