#ifndef __MATRIX_H__
#define __MATRIX_H__

#define MATRIX_RELEASE		0
#define MATRIX_VERSION		1

#ifndef INLINE
#define INLINE	inline
#endif

#define STATIC_ASSERT(expr)	typedef char assert_type[(expr) ? 1 : -1];

#define FLAG_ON(_x, _f)		((_x) & (_f))

#define SET_FLAG(_x, _f)	((_x) |= (_f))

#define CLEAR_FLAG(_x, _f)	((_x) &= ~(_f))

#define MIN(_x, _y)		((_x) < (_y) ? (_x) : (_y))

#define MAX(_x, _y)		((_x) > (_y) ? (_x) : (_y))

#define ROUND_UP(_x, _y)	(((_x) + (_y) - 1) & ~((_y) - 1))

#define ROUND_DOWN(_x, _y)	((_x) & ~((_y) - 1))

#endif	/* __MATRIX_H__ */
