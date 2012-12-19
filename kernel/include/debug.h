#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "hal/hal.h"
#include "util.h"
#include "kd.h"

/* Compiler macro to get function name */
#ifndef __FUNC__
#define __FUNC__	__func__
#endif	/* __FUNC__ */

#define DL_DBG		0x00000001
#define DL_INF		0x00000002
#define DL_WRN		0x00000003
#define DL_ERR		0x00000004

#define DL_MAX		DL_ERR

#define DEBUG(level, params) do { \
		if (_debug_level <= (level)) { \
			boolean_t state;						\
			state = irq_disable();						\
			if (_debug_level > DL_DBG) {					\
				kprintf("[%s] %s: ", dbglevel_string(level), __FUNC__);	\
				kprintf params;						\
			}								\
			kd_printf("[%s] %s: ", dbglevel_string(level), __FUNC__);	\
			kd_printf params;						\
			irq_restore(state);						\
		}									\
	} while (0)


#define PANIC(msg)	panic(__FILE__, __LINE__, msg)

#define ASSERT(b)	((b) ? (void)0: panic_assert(__FILE__, __LINE__, #b))


extern uint32_t _debug_level;

extern const char *dbglevel_string(uint32_t level);
extern void panic(const char *file, uint32_t line, const char *msg);
extern void panic_assert(const char *file, uint32_t line, const char *desc);

#endif	/* __DEBUG_H__ */
