#ifndef __PRIV_H__
#define __PRIV_H__

struct priv {
	uint32_t flags;
};

/* Flags for the system privileges structure */
#define PREEMPTIBLE	0x00000001	// Kernel tasks are not preemptible
#define BILLABLE	0x00000002	// Some tasks are not billable

#endif	/* __PRIV_H__ */
