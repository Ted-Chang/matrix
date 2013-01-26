#ifndef __MODULE_H__
#define __MODULE_H__

#define KMOD_RAMFS	1
#define KMOD_DEVFS	2
#define KMOD_PROCFS	3

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
extern void init_module();

#endif	/* __MODULE_H__ */
