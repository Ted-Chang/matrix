#ifndef __LAPIC_H__
#define __LAPIC_H__

/* Local APIC register offsets */
#define LAPIC_REG_APIC_ID		0x20	// Local APIC ID
#define LAPIC_REG_APIC_VERSION		0x30	// Local APIC Version
#define LAPIC_REG_TPR			0x80	// Task Priority Register (TPR)
#define LAPIC_REG_APR			0x90	// Arbitration Priority Register (APR)
#define LAPIC_REG_PPR			0xA0	// Processor Priority Register (PPR)
#define LAPIC_REG_EOI			0xB0	// EOI
#define LAPIC_REG_LOGICAL_DEST		0xD0	// Logical Destination
#define LAPIC_REG_DEST_FORMAT		0xE0	// Destination Format
#define LAPIC_REG_SPURIOUS		0xF0	// Spurious Interrupt Vector
#define LAPIC_REG_ISR0			0x100	// In-Service Register (ISR) - bits 0:31
#define LAPIC_REG_ISR1			0x110	// In-Service Register (ISR) - bits 32:63
#define LAPIC_REG_ICR0			0x300	// Interrupt Command Register (ICR) - bits 0:31
#define LAPIC_REG_ICR1			0x310	// Interrupt Command Register (ICR) - bits 32:63
#define LAPIC_REG_ERROR			0x280	// Error Status Register (ESR)
#define LAPIC_REG_LVT_TIMER		0x320	// LVT Timer
#define LAPIC_REG_LVT_PERFMON		0x340	// LVT Performance Monitoring Counters
#define LAPIC_REG_LVT_LINT0		0x350	// LVT LINT0
#define LAPIC_REG_LVT_LINT1		0x360	// LVT LINT1
#define LAPIC_REG_LVT_ERROR		0x370	// LVT Error
#define LAPIC_REG_TIMER_INITIAL		0x380	// Timer Initial Count
#define LAPIC_REG_TIMER_CURRENT		0x390	// Timer Current Count
#define LAPIC_REG_TIMER_DIVIDER		0x3E0	// Timer Divide Configuration

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

/* IPI delivery modes */
#define LAPIC_IPI_FIXED			0x00	// Fixed (vector specified)
#define LAPIC_IPI_NMI			0x04	// NMI
#define LAPIC_IPI_INIT			0x05	// INIT
#define LAPIC_IPI_SIPI			0x06	// Start-Up (SIPI)

/* IPI destination shorthands */
#define LAPIC_IPI_DEST_SINGLE		0x00	// Send to a single CORE (dest ID specified)
#define LAPIC_IPI_DEST_ALL		0x03	// All, excluding self
#define LAPIC_IPI_DEST_ALL_INCL		0x02	// All, including self

extern void lapic_timer_prepare(useconds_t us);
extern boolean_t lapic_enabled();
extern uint32_t lapic_id();
extern void lapic_ipi(uint8_t dest, uint8_t id, uint8_t mode, uint8_t vector);
extern void init_lapic();

#endif	/* __LAPIC_H__ */
