#include <types.h>
#include <stddef.h>
#include "hal/spinlock.h"
#include "mutex.h"

uint32_t _test_global_var;

void lock_test()
{
	struct spinlock l;
	struct mutex m;
	int i;

	_test_global_var = 0;
	
	spinlock_init(&l, "test_lock");

	for (i = 0; i < 128; i++) {
		
		spinlock_acquire(&l);
		_test_global_var++;
		spinlock_release(&l);

		spinlock_acquire(&l);
		_test_global_var--;
		spinlock_release(&l);
	}

	kprintf("spinlock test OK!\n");

	mutex_init(&m, "test_mutex", 0);
	
	for (i = 0; i < 128; i++) {
		mutex_acquire(&m);
		_test_global_var++;
		mutex_release(&m);

		mutex_acquire(&m);
		_test_global_var--;
		mutex_release(&m);
	}

	kprintf("mutex test OK!\n");
}
