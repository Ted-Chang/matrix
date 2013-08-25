#include <types.h>
#include "debug.h"
#include "platform.h"

void init_smp()
{
	/* Detect application CORE */
	platform_detect_smp();
}
