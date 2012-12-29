#include <types.h>
#include <stddef.h>
#include <string.h>
#include "matrix/matrix.h"
#include "matrix/module.h"
#include "list.h"
#include "mm/malloc.h"
#include "mutex.h"
#include "module.h"
#include "debug.h"

// TODO: The following functions should be get from the driver files
extern int initrd_init(void);
extern int keyboard_init(void);
extern int floppy_init(void);
extern int devfs_init(void);

/* List of loaded modules */
static struct list _module_list = {
	.prev = &_module_list,
	.next = &_module_list
};
static struct mutex _module_lock;

// TODO: The following function should be removed in the future
static int load_module_stub(struct module *m)
{
	int rc = 0;

	switch (m->handle) {
	case KMOD_RAMFS:
		m->name = "ramfs";
		m->desc = "Ram File System";
		m->init = initrd_init;
		break;
	case KMOD_KBD:
		m->name = "kbd";
		m->desc = "Keyboard driver";
		m->init = keyboard_init;
		break;
	case KMOD_FLPY:
		m->name = "flpy";
		m->desc = "Floppy driver";
		m->init = floppy_init;
		break;
	case KMOD_DEVFS:
		m->name = "devfs";
		m->desc = "Device File System";
		m->init = devfs_init;
		break;
	default:
		rc = -1;
	}

	return rc;
}

static struct module *module_alloc(int handle)
{
	struct module *m;

	m = kmalloc(sizeof(struct module), 0);
	if (m) {
		LIST_INIT(&m->link);
		m->ref_count = 0;
		m->handle = handle;
	}

	return m;
}

static void module_free(struct module *m)
{
	kfree(m);
}

static struct module *module_lookup(const char *name)
{
	struct list *l;
	struct module *mod;

	LIST_FOR_EACH(l, &_module_list) {
		mod = LIST_ENTRY(l, struct module, link);
		if (strcmp(mod->name, name) == 0) {
			return mod;
		}
	}

	return NULL;
}

int module_load(int handle)
{
	int rc = -1;
	struct module *m;
	
	/* Acquire the module lock */
	mutex_acquire(&_module_lock);

	m = module_alloc(handle);
	rc = load_module_stub(m);
	if (rc != 0) {
		goto out;
	}

	/* Check whether the module is valid */
	if (!m->name || !m->desc || !m->init) {
		rc = -1;
		goto out;
	}

	/* Check if the module with this name already loaded */
	if (NULL != module_lookup(m->name)) {
		rc = -1;
		goto out;
	}

	rc = m->init();
	if (rc != 0) {
		goto out;
	}

	list_add_tail(&m->link, &_module_list);
	m->handle = 0;

	DEBUG(DL_DBG, ("load module(%s:%s) successfully.\n",
		       m->name, m->desc));

 out:
	if (rc != 0) {
		if (m) {
			module_free(m);
		}
	}
	mutex_release(&_module_lock);

	return rc;
}

void init_module()
{
	mutex_init(&_module_lock, "mod-mutex", 0);
}
