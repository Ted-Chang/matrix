#ifndef __STDARG_H__
#define __STDARG_H__

#include <va_list.h>

/* Width of stack == width of int */
#define STACKITEM	int

/* Round up width of objects pushed on stack. The expression before the 
 * & ensures that we get 0 for objects of size 0.
 */
#define VA_SIZE(TYPE) \
	((sizeof(TYPE) + sizeof(STACKITEM) - 1) & ~(sizeof(STACKITEM) - 1))

/* &(LASTARG) points to the LEFTMOST argument of the function call
 * (before the ...)
 */
#define va_start(AP, LASTARG) \
	(AP=((va_list)&(LASTARG) + VA_SIZE(LASTARG)))

/* Nothing for va_end */
#define va_end(AP)

#define va_arg(AP, TYPE) \
	(*(TYPE *)((AP += VA_SIZE(TYPE)) - VA_SIZE(TYPE)))

int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int vsprintf(char *buf, const char *fmt, va_list args);

#endif	/* __STDARG_H__ */
