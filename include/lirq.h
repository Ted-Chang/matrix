#ifndef __LIRQ_H__
#define __LIRQ_H__

#include <types.h>
#include "matrix/matrix.h"

static INLINE boolean_t local_irq_enable()
{
	u_long flags;

	asm volatile("pushf; sti; pop %0" : "=r"(flags));
	return (flags & (1 << 9)) ? TRUE : FALSE;
}

static INLINE boolean_t local_irq_disable()
{
	u_long flags;

	asm volatile("pushf; cli; pop %0" : "=r"(flags));
	return (flags & (1 << 9)) ? TRUE : FALSE;
}

static INLINE void local_irq_restore(boolean_t state)
{
	if (state) {
		asm volatile("sti");
	} else {
		asm volatile("cli");
	}
}

static INLINE boolean_t local_irq_state()
{
	u_long flags;

	asm volatile("pushf; pop %0" : "=r"(flags));
	return (flags & (1 << 9)) ? TRUE : FALSE;
}

#endif	/* __LIRQ_H__ */
