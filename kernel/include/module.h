#ifndef __MODULE_H__
#define __MODULE_H__

#define KMOD_RAMFS	1
#define KMOD_DEVFS	2
#define KMOD_PROCFS	3
#define KMOD_PCI	4
#define KMOD_KBD	5
#define KMOD_FLPY	6
#define KMOD_NULL	7

typedef int (*module_init_func_t)(void);

typedef int (*module_unload_func_t)(void);

struct module {
	struct list link;		// Link to loaded modules list

	int ref_count;			// Reference count of this module
	int handle;			// Handle to module file

	/* Module information */
	const char *name;
	const char *desc;
	module_init_func_t init;
	module_unload_func_t unload;
};

extern int module_load(int handle);
extern int module_unload(const char *name);
extern void init_module();

#endif	/* __MODULE_H__ */
