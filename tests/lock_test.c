#include <types.h>
#include <stddef.h>
#include "hal/spinlock.h"

uint32_t _test_global_var;

void lock_test()
{
	struct spinlock lock;

	_test_global_var = 0;
	
	spinlock_init(&lock, "test_lock");
	
	spinlock_acquire(&lock);

	_test_global_var++;
	
	spinlock_release(&lock);
}
