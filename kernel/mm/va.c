#include <types.h>
#include <stddef.h>
#include "debug.h"
#include "hal/core.h"
#include "mm/kmem.h"
#include "mm/va.h"

struct va_space *va_create()
{
	struct va_space *vas;

	vas = kmem_alloc(sizeof(struct va_space), 0);
	if (vas) {
		vas->mmu = mmu_create_ctx();
		if (!vas->mmu) {
			kmem_free(vas);
			vas = NULL;
		}
	}
	
	return vas;
}

int va_map(struct va_space *vas, ptr_t start, size_t size, int flags, ptr_t *addrp)
{
	int rc;
	
	if (!size || (size % PAGE_SIZE)) {
		DEBUG(DL_DBG, ("size (%x) invalid.\n", size));
		rc = -1;
		goto out;
	}
	
	if (flags & MAP_FIXED_F) {
		if (start % PAGE_SIZE) {
			DEBUG(DL_DBG, ("start(%p) not aligned.\n", start));
			rc = -1;
			goto out;
		}
	} else /*if (!addrp)*/ {
		/* We didn't support this yet. */
		DEBUG(DL_DBG, ("non-fixed map not support yet.\n"));
		rc = -1;
		goto out;
	}

	DEBUG(DL_DBG, ("vas(%p) start(%p), size(%x).\n", vas, start, size));
	
	rc = mmu_map(vas->mmu, start, size, flags, NULL);

 out:
	return rc;
}

int va_unmap(struct va_space *vas, ptr_t start, size_t size)
{
	int rc;
	
	if (!size || (start % PAGE_SIZE) || (size % PAGE_SIZE)) {
		rc = -1;
		goto out;
	}

	rc = mmu_unmap(vas->mmu, start, size);

 out:
	return rc;
}

void va_switch(struct va_space *vas)
{
	boolean_t state;

	/* The kernel process does not have an address space. When switching
	 * to one of its threads, it is not necessary to switch to the kernel
	 * address space, as all mappings in the kernel context are visible in
	 * all address spaces. Kernel threads should never touch the userspace
	 * portion of the address space.
	 */
	if (vas && (vas != CURR_ASPACE)) {
		
		DEBUG(DL_DBG, ("new vas(%p), current vas(%p), core(%p).\n",
			       vas, CURR_ASPACE, CURR_CORE));

		state = irq_disable();

		/* Update the current mmu context */
		CURR_ASPACE = vas;

		/* Load the page directory base */
		mmu_load_ctx(vas->mmu);

		irq_restore(state);
	}
}

void va_clone(struct va_space *dst, struct va_space *src)
{
	;
}

void va_destroy(struct va_space *vas)
{
	mmu_destroy_ctx(vas->mmu);
	kmem_free(vas);
}

void init_va()
{
	;
}

