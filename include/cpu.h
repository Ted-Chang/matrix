#ifndef __CPU_H__
#define __CPU_H__

/* CPU ID */
typedef uint32_t cpu_id_t;

struct cpu {
	/* Current state of the CPU */
	enum {
		CPU_OFFLINE,
		CPU_RUNNING,
	} state;
};

void init_cpu();

#endif	/* __CPU_H__ */
