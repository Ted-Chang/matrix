#ifndef __SMP_H__
#define __SMP_H__

#include "hal/core.h"

typedef int (*smp_call_func_t)(void *ctx);

extern volatile uint32_t _smp_boot_status;

/* Values for _smp_boot_status */
#define SMP_BOOT_INIT		0	// Boot started
#define SMP_BOOT_ALIVE		1	// AC has reached kmain_ac()
#define SMP_BOOT_BOOTED		2	// AC has completed kmain_ac()
#define SMP_BOOT_COMPLETE	3	// All ACs have been booted

extern void smp_ipi_handler();
extern void init_smp();

#endif
