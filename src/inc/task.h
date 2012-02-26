#ifndef __TASK_H__
#define __TASK_H__

struct task {
	int id;
	uint32_t esp;
	uint32_t ebp;
	uint32_t eip;
	struct pd *page_dir;
	struct task *next;
};

void init_multitask();

void task_switch();

int fork();

int getpid();

#endif	/* __TASK_H__ */
