#include <types.h>
#include <stddef.h>
#include "mm/vaspace.h"

void va_space_switch(struct va_space *space)
{
	boolean_t state;

	/* The kernel process does not have an address space. When switching to
	 * one of its threads, it is not necessary to switch to the kernel MMU
	 * context, as all mappings in the kernel context are visable in all
	 * address space.
	 */
	if (space && space != CURR_SPACE) {
		state = irq_disable();

		/* Switch to the new virtual address space */
		mmu_switch_ctx(space->mmu);
		CURR_SPACE = space;

		irq_restore(state);
	}
}

struct va_space *va_space_create()
{
	struct va_space *space;

	space = kmalloc(sizeof(struct va_space));
	space->mmu = mmu_create_ctx();
	mutex_init(&space->lock);

	return space;
}

struct va_space *va_space_clone(struct va_space *src)
{
	struct va_space *space;

	space = kmalloc(sizeof(struct va_space));
	space->mmu = mmu_create_ctx();
	mutex_init(&space->lock);

	mutex_acquire(&src->lock);

	mmu_copy_ctx(space->mmu, src->mmu);
	
	mutex_release(&src->lock);

	return space;
}

void va_space_destroy(struct va_space *space)
{
	ASSERT(space);

	/* Destroy the MMU context */
	mmu_destroy_ctx(space->mmu);
}

void init_va()
{
	;
}
