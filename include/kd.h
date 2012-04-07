#ifndef __KDB_H__
#define __KDB_H__

typedef int kd_status_t;

#define KD_SUCCESS		0x00000000
#define KD_FAILURE		0x00000001
#define KD_CONTINUE		0x00000002
#define KD_STEP			0x00000003


typedef int kdb_reason_t;

#define KD_REASON_USER		0x00000000
#define KD_REASON_FATAL		0x00000001
#define KD_REASON_BREAK		0x00000002
#define KD_REASON_WATCH		0x00000003
#define KD_REASON_STEP		0x00000004

/* Kernel Debugger output filter structure */
struct kd_filter {
	boolean_t (*func)(const char *line, void *data);
	void *data;
};

typedef kd_status_t (*kd_cmd_t)(int argc, char **argv, struct kd_filter *filter);

#endif	/* __KDB_H__ */
