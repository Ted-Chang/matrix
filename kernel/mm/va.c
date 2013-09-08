#include <types.h>
#include <stddef.h>
#include "debug.h"
#include "hal/core.h"
#include "mm/page.h"
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
	int pflag = 0;
	struct page *p;
	ptr_t virt;

	if (!size || (size % PAGE_SIZE)) {
		DEBUG(DL_DBG, ("size (%x) invalid.\n", size));
		rc = -1;
		goto out;
	}
	
	if (flags & VA_MAP_FIXED) {
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
	
	for (virt = start; virt < (start + size); virt += PAGE_SIZE) {
		p = mmu_get_page(vas->mmu, virt, TRUE, 0);
		if (!p) {
			DEBUG(DL_DBG, ("mmu_get_page failed, addr(%p).\n", virt));
			rc = -1;
			goto out;
		}
		
		DEBUG(DL_DBG, ("mmu(%p) page(%p) frame(%x).\n", vas->mmu, p, p->frame));
		page_alloc(p, pflag);
		p->user = IS_KERNEL_CTX(vas->mmu) ? FALSE : TRUE;
		p->rw = FLAG_ON(flags, VA_MAP_WRITE) ? TRUE : FALSE;
	}

	rc = 0;
	
 out:
	return rc;
}

int va_unmap(struct va_space *vas, ptr_t start, size_t size)
{
	int rc;
	ptr_t virt;
	struct page *p;

	if (!size || (start % PAGE_SIZE) || (size % PAGE_SIZE)) {
		rc = -1;
		goto out;
	}

	for (virt = start; virt < start + size; virt += PAGE_SIZE) {
		p = mmu_get_page(vas->mmu, virt, FALSE, 0);
		if (!p) {
			rc = -1;
			goto out;
		}
		
		DEBUG(DL_DBG, ("mmu(%p) page(%p) frame(%x).\n", vas->mmu, p, p->frame));
		page_free(p);
	}

	rc = 0;

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
#ifdef _DEBUG_MM
		DEBUG(DL_DBG, ("new vas(%p), current vas(%p), core(%d).\n",
			       vas, CURR_ASPACE, CURR_CORE->id));
#endif	/* _DEBUG_MM */

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

