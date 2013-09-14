#include <types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include "matrix/matrix.h"
#include "matrix/module.h"
#include "list.h"
#include "mm/malloc.h"
#include "mutex.h"
#include "module.h"
#include "debug.h"

// TODO: The following functions should be get from the driver files
extern int initrd_init(void);
extern int devfs_init(void);
extern int procfs_init(void);
extern int pci_init(void);
extern int keyboard_init(void);
extern int floppy_init(void);

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
		m->unload = NULL;
		break;
	case KMOD_DEVFS:
		m->name = "devfs";
		m->desc = "Device File System";
		m->init = devfs_init;
		m->unload = NULL;
		break;
	case KMOD_PROCFS:
		m->name = "procfs";
		m->desc = "Process File System";
		m->init = procfs_init;
		m->unload = NULL;
		break;
	case KMOD_PCI:
		m->name = "pci";
		m->desc = "PCI bus driver";
		m->init = pci_init;
		m->unload = NULL;
		break;
	case KMOD_KBD:
		m->name = "kbd";
		m->desc = "Keyboard driver";
		m->init = keyboard_init;
		m->unload = NULL;
		break;
	case KMOD_FLPY:
		m->name = "flpy";
		m->desc = "Floppy disk driver";
		m->init = floppy_init;
		m->unload = NULL;
		break;
	default:
		DEBUG(DL_INF, ("unknown module(%s:%d).\n", m->name, m->handle));
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
	if (!m) {
		rc = ENOMEM;
		goto out;
	}
	
	rc = load_module_stub(m);
	if (rc != 0) {
		goto out;
	}

	/* Check whether the module is valid */
	if (!m->name || !m->desc || !m->init) {
		rc = EINVAL;
		goto out;
	}

	/* Check if the module with this name already loaded */
	if (NULL != module_lookup(m->name)) {
		DEBUG(DL_INF, ("module(%s) already loaded.\n", m->name));
		rc = -1;
		goto out;
	}

	/* Call the module's init routine */
	rc = m->init();
	if (rc != 0) {
		DEBUG(DL_INF, ("module(%s) init failed.\n", m->name));
		goto out;
	}

	list_add_tail(&m->link, &_module_list);
	m->handle = 0;
	m->ref_count++;

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

int module_unload()
{
	int rc = -1;

	mutex_acquire(&_module_lock);

	mutex_release(&_module_lock);

	return rc;
}

void init_module()
{
	mutex_init(&_module_lock, "mod-mutex", 0);
}
