#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "util.h"

#define DL_ERR		0x00000004
#define DL_WRN		0x00000003
#define DL_INF		0x00000002
#define DL_DBG		0x00000001

#define DEBUG(level, params) do { \
		if (debug_level >= (level)) { \
			kprintf("[DEBUG] ");  \
			kprintf params;	      \
		}			      \
	} while (0)

extern uint32_t debug_level;

#ifdef _X86_
#define dbg_break()	asm volatile("int $0x3")
#else
#define dbg_break()
#endif	/* _X86_ */

#endif	/* __DEBUG_H__ */
