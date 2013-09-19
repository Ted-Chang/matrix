#ifndef __SMP_H__
#define __SMP_H__

#include "hal/core.h"

typedef int (*smp_call_func_t)(void *ctx);

extern void init_smp();

#endif
