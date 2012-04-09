#ifndef __LAPIC_H__
#define __LAPIC_H__

#define LAPIC_REG_APIC_ID	8
#define LAPIC_REG_APIC_VERSION	12
#define LAPIC_REG_TPR		32
#define LAPIC_REG_APR		36
#define LAPIC_REG_PPR		40
#define LAPIC_REG_EOI		44
#define LAPIC_REG_LOGICAL_DEST	52
#define LAPIC_REG_DEST_FORMAT	56
#define LAPIC_REG_SPURIOUS	60
#define LAPIC_REG_ISR0		64
#define LAPIC_REG_ISR1		68
#define LAPIC_REG_ISR2		72
#define LAPIC_REG_ISR3		76
#define LAPIC_REG_ISR4		80
#define LAPIC_REG_ISR5		84
#define LAPIC_REG_ISR6		88
#define LAPIC_REG_ISR7		92
#define LAPIC_REG_TMR0		96
#define LAPIC_REG_TMR1		100
#define LAPIC_REG_TMR2		104
#define LAPIC_REG_TMR3		108
#define LAPIC_REG_TMR4		112
#define LAPIC_REG_TMR5		116
#define LAPIC_REG_TMR6		120
#define LAPIC_REG_TMR7		124
#define LAPIC_REG_IRR0		128
#define LAPIC_REG_IRR1		132
#define LAPIC_REG_IRR2		136
#define LAPIC_REG_IRR3		140
#define LAPIC_REG_IRR4		144
#define LAPIC_REG_IRR5		148
#define LAPIC_REG_IRR6		152
#define LAPIC_REG_IRR7		156
#define LAPIC_REG_ERROR		160
#define LAPIC_REG_ICR0		192
#define LAPIC_REG_ICR1		196
#define LAPIC_REG_LVT_TIMER	200
#define LAPIC_REG_LVT_THERMAL	204
#define LAPIC_REG_LVT_PERFMON	208
#define LAPIC_REG_LVT_LINT0	212
#define LAPIC_REG_LVT_LINT1	216
#define LAPIC_REG_LVT_ERROR	220
#define LAPIC_REG_TIMER_INITIAL	224
#define LAPIC_REG_TIMER_CURRENT	228
#define LAPIC_REG_TIMER_DIVIDER	248

/* Local APIC Divide Configuration Register values */
#define LAPIC_TIMER_DIV1	0xB
#define LAPIC_TIMER_DIV2	0x0
#define LAPIC_TIMER_DIV4	0x1
#define LAPIC_TIMER_DIV8	0x2
#define LAPIC_TIMER_DIV16	0x3
#define LAPIC_TIMER_DIV32	0x8
#define LAPIC_TIMER_DIV64	0x9
#define LAPIC_TIMER_DIV128	0xA

/* Local APIC interrupt vectors */
#define LAPIC_VECT_TIMER	0xF0	// Timer
#define LAPIC_VECT_SPURIOUS	0xF1	// Spurious
#define LAPIC_VECT_IPI		0xF2	// IPI message

/* IPI delivery modes */
#define LAPIC_IPI_FIXED		0x00
#define LAPIC_IPI_NMI		0x04
#define LAPIC_IPI_INIT		0x05
#define LAPIC_IPI_SIPI		0x06

/* IPI destination shorthands */
#define LAPIC_IPI_DEST_SINGLE	0x00
#define LAPIC_IPI_DEST_ALL	0x03
#define LAPIC_IPI_DEST_ALL_INCL	0x02	// All, including self

extern boolean_t lapic_enabled();
extern void init_lapic();
extern uint32_t lapic_id();

#endif	/* __LAPIC_H__ */
