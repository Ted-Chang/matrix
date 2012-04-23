#include <types.h>
#include <stddef.h>
#include "list.h"
#include "hal/spinlock.h"
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "mm/page.h"
#include "mm/mlayout.h"
#include "mm/mm.h"
#include "mm/phys.h"

void *physical_map(phys_addr_t addr, size_t size, int flags)
{
	return NULL;
}
