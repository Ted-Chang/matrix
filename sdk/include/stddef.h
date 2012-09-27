#ifndef __STDDEF_H__
#define __STDDEF_H__

#ifndef NULL
#define NULL	((void *)0)
#endif

typedef unsigned size_t;

#ifdef _AMD64_
#define offsetof(s,m)	(size_t)((ptrdiff_t)&(((s *)0)->m) )
#else
#define offsetof(s,m)	(size_t)&(((s *)0)->m)
#endif	/* _AMD64_ */

#endif	/* __STDDEF_H__ */
