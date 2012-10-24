#ifndef __NOTIFIER_H__
#define __NOTIFIER_H__

#include "list.h"

struct notifier {
	struct list functions;	// Functions to call when the event was signaled
};
typedef struct notifier notifier_t;

extern void init_notifier(struct notifier *n);
extern void notifier_clear(struct notifier *n);
extern void notifier_run(struct notifier *n);
extern void notifier_register(struct notifier *n, void (*func)(void *), void *data);
extern void notifier_unregister(struct notifier *n, void (*func)(void *), void *data);

#endif	/* __NOTIFIER_H__ */
