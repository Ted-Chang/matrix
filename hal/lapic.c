#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "list.h"
#include "div64.h"
#include "mm/mm.h"
#include "mm/page.h"
#include "mm/phys.h"
#include "cpu.h"
#include "time.h"
#include "pit.h"
#include "hal/lapic.h"
#include "hal/hal.h"

/* Local APIC mapping. If NULL the LAPIC is not present */
static volatile uint32_t *_lapic_mapping = NULL;

/* Local APIC base address */
static uint32_t _lapic_base = 0;

/* Spurious IRQ hook and timer IRQ hook */
static struct irq_hook _lapic_spurious_hook;
static struct irq_hook _lapic_timer_hook;


static INLINE uint32_t lapic_read(unsigned reg)
{
	return _lapic_mapping[reg];
}

static INLINE void lapic_write(unsigned reg, uint32_t value)
{
	_lapic_mapping[reg] = value;
}

static INLINE void lapic_eoi()
{
	lapic_write(LAPIC_REG_EOI, 0);
}

static void lapic_spurious_callback(struct intr_frame *frame)
{
	DEBUG(DL_DBG, ("lapic_spurious_callback: spurious interrupt received.\n"));
}

static void lapic_timer_init(useconds_t us)
{
	uint32_t count = (uint32_t)((CURR_CPU->arch.lapic_timer_cv * us) >> 32);
	lapic_write(LAPIC_REG_TIMER_INITIAL, (count == 0 && us != 0) ? 1 : count);
}

static void lapic_timer_callback(struct intr_frame *frame)
{
	do_clocktick();
	lapic_eoi();
}

boolean_t lapic_enabled()
{
	return (_lapic_mapping != NULL);
}

uint32_t lapic_id()
{
	return _lapic_mapping ? (lapic_read(LAPIC_REG_APIC_ID) >> 24) : 0;
}

static uint32_t calculate_lapic_freq()
{
	uint16_t shi, slo, ehi, elo, pticks;
	uint32_t end, lticks;
		
	/* Set PIT channel 0 to single-shot mode */
	outportb(0x43, 0x34);
	outportb(0x40, 0xFF);
	outportb(0x40, 0xFF);

	/* Wait for the cycle to begin */
	do {
		outportb(0x43, 0x00);
		slo = inportb(0x40);
		shi = inportb(0x40);
	} while (shi != 0xFF);

	/* Kick off the LAPIC timer */
	lapic_write(LAPIC_REG_TIMER_INITIAL, 0xFFFFFFFF);

	/* Wait for the high byte to decrease to 128 */
	do {
		outportb(0x43, 0x00);
		elo = inportb(0x40);
		ehi = inportb(0x40);
	} while (ehi > 0x80);

	/* Get current timer value */
	end = (uint32_t)lapic_read(LAPIC_REG_TIMER_CURRENT);

	/* LAPIC ticks */
	lticks = 0xFFFFFFFF - end;
	/* PIT ticks */
	pticks = ((ehi << 8) | elo) - ((shi << 8) | slo);

	/* Calculate frequency */
	ASSERT(PIT_BASE_FREQUENCY > pticks);
	return lticks * 8 * (PIT_BASE_FREQUENCY / pticks);
}

/**
 * Initialize the local APIC on the current CPU.
 */
void init_lapic()
{
	uint64_t base, temp;

	/* Don't do anything if we don't have LAPIC support */
	if (!_cpu_features.apic)
		return;

	/* Get the base address of the LAPIC mapping. If bit 11 is 0, the LAPIC
	 * is disabled.
	 */
	base = x86_read_msr(X86_MSR_APIC_BASE);
	if (!(base & (1 << 11)))
		return;
	else if (_cpu_features.x2apic && (base & (1 << 10)))
		PANIC("Cannot handle CPU in x2APIC mode");

	/* Determines the physical address of the APIC registers page */
	base &= 0xFFFFF000;

	if (_lapic_mapping) {
		/* This is a secondary CPU. Ensure that the base address is not
		 * different to the boot CPU's.
		 */
		if (base != _lapic_base) 
			PANIC("This CPU has different LAPIC address to boot CPU");
	} else {
		/* This is the boot CPU. Map the LAPIC into virtual memory and
		 * register interrupt vector handlers.
		 */
		_lapic_base = base;
		_lapic_mapping = kmem_map((phys_addr_t)base, PAGE_SIZE, 0);

		DEBUG(DL_DBG, ("lapic: physical location 0x%016lx, mapped to 0x%x\n",
			       base, _lapic_mapping));

		/* Install interrupt vector handlers */
		register_irq_handler(LAPIC_VECT_SPURIOUS, &_lapic_spurious_hook,
				     lapic_spurious_callback);
		register_irq_handler(LAPIC_VECT_TIMER, &_lapic_timer_hook,
				     lapic_timer_callback);
	}

	/* Enable the local APIC (bit 8) and set the spurious interrupt vector
	 * in the Spurious Interrupt Vector Register.
	 */
	lapic_write(LAPIC_REG_SPURIOUS, LAPIC_VECT_SPURIOUS|(1<<8));
	lapic_write(LAPIC_REG_TIMER_DIVIDER, LAPIC_TIMER_DIV8);

	/* Calculate LAPIC frequency. */
	if (CURR_CPU == &_boot_cpu)
		CURR_CPU->arch.lapic_freq = calculate_lapic_freq();
	else
		CURR_CPU->arch.lapic_freq = _boot_cpu.arch.lapic_freq;

	/* Sanity check */
	if (CURR_CPU != &_boot_cpu) {
		if (CURR_CPU->id != lapic_id())
			PANIC("CPU ID mismatch detected");
	}

	/* Figure out the timer conversion factor */
	temp = ((uint64_t)(CURR_CPU->arch.lapic_freq / 8)) << 32;
	do_div(temp, 1000000);
	CURR_CPU->arch.lapic_timer_cv = temp;
	
	DEBUG(DL_DBG, ("timer conversion factor for CPU(%d):%d, frequency(%d MHz)\n",
		       CURR_CPU->id, CURR_CPU->arch.lapic_timer_cv,
		       CURR_CPU->arch.lapic_freq / 1000000));
	
	/* Accept all interrupts */
	lapic_write(LAPIC_REG_TPR, lapic_read(LAPIC_REG_TPR) & 0xFFFFFF00);

	/* Enable the timer: interrupt vector, no extra bits */
	lapic_write(LAPIC_REG_TIMER_INITIAL, 0);
	lapic_write(LAPIC_REG_LVT_TIMER, LAPIC_VECT_TIMER);
}
