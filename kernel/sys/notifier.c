#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "matrix/debug.h"
#include "mm/malloc.h"
#include "notifier.h"

struct notifier_func {
	struct list link;	// Link to other functions
	void (*func)(void *);	// Function to call
	void *data;		// Data argument for function
};

void init_notifier(struct notifier *n)
{
	LIST_INIT(&n->functions);
}

void notifier_clear(struct notifier *n)
{
	struct list *l;
	struct notifier_func *nf;

	LIST_FOR_EACH(l, &n->functions) {
		nf = LIST_ENTRY(l, struct notifier_func, link);
		list_del(&nf->link);
		kfree(nf);
	}
}

void notifier_run(struct notifier *n)
{
	struct list *l, *p;
	struct notifier_func *nf;

	LIST_FOR_EACH_SAFE(l, p, &n->functions) {
		nf = LIST_ENTRY(l, struct notifier_func, link);

		DEBUG(DL_DBG, ("notifier_run: nf(%p), func(%p), data(%p).\n",
			       nf, nf->func, nf->data));

		ASSERT((nf != NULL) && (nf->func != NULL) && (nf->data != NULL));
		
		list_del(&nf->link);
		nf->func(nf->data);
		kfree(nf);
	}
}

void notifier_register(struct notifier *n, void (*func)(void *), void *data)
{
	struct notifier_func *nf;

	nf = kmalloc(sizeof(struct notifier), 0);
	LIST_INIT(&nf->link);
	nf->data = data;
	nf->func = func;

	list_add_tail(&nf->link, &n->functions);

	DEBUG(DL_DBG, ("notifier_register: nf(%p), func(%p), data(%p).\n",
		       nf, nf->func, nf->data));
}

void notifier_unregister(struct notifier *n, void (*func)(void *), void *data)
{
	struct list *l;
	struct notifier_func *nf;

	LIST_FOR_EACH(l, &n->functions) {
		nf = LIST_ENTRY(l, struct notifier_func, link);
		if (nf->data == data) {
			list_del(&nf->link);
			kfree(nf);
			break;
		}
	}
}
