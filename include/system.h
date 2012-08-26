#ifndef __SYSTEM_H__
#define __SYSTEM_H__

typedef void (*task_func_t)(void *);

extern task_func_t images[];

extern uint32_t _nr_boot_tasks;

#define NR_BOOT_TASKS	_nr_boot_tasks

#endif	/* __SYSTEM_H__ */
