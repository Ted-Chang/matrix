#ifndef __KD_H__
#define __KD_H__

struct kd_filter {
	int (*func)(const char *line, void *data);

	void *data;	// Data for the filter
};
typedef struct kd_filter kd_filter_t;

typedef int (*kd_cmd_t)(int argc, char **argv, kd_filter_t *filter);

extern void kd_printf(const char *fmt, ...);
extern void kd_register_cmd(const char *name, const char *desc, kd_cmd_t func);
extern void kd_unregister_cmd(const char *name);
extern void init_kd();

#endif	/* __KD_H__ */
