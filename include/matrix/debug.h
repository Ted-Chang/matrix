#ifndef __DEBUG_H__
#define __DEBUG_H__

#define DL_ERR		0x00000004
#define DL_WRN		0x00000003
#define DL_INF		0x00000002
#define DL_DBG		0x00000001

#define DEBUG(level, params) do { \
		if (_debug_level >= (level)) { \
			kprintf("[DEBUG] ");   \
			kprintf params;	       \
		}			       \
	} while (0)


#define PANIC(msg)	panic(__FILE__, __LINE__, msg)

#define ASSERT(b)	((b) ? (void)0: panic_assert(__FILE__, __LINE__, #b))


extern uint32_t _debug_level;

extern void putch(char ch);
extern void clear_scr();
extern void putstr(const char *str);
extern int kprintf(const char *str, ...);
extern void panic(const char *file, uint32_t line, const char *message);
extern void panic_assert(const char *file, uint32_t line, const char *desc);

#ifdef _DEBUG_SCHED
void check_runqueues(char *when);
#endif	/* _DEBUG_SCHED */

#endif	/* __DEBUG_H__ */
