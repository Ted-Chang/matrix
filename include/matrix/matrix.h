#ifndef __MATRIX_H__
#define __MATRIX_H__

#define MATRIX_RELEASE		0
#define MATRIX_VERSION		1

#ifndef INLINE
#define INLINE	inline
#endif

#define FLAG_ON(_x, _f)		((_x) & (_f))

#define SET_FLAG(_x, _f)	((_x) |= (_f))

#define CLEAR_FLAG(_x, _f)	((_x) &= ~(_f))

#define ROUND_UP(_x, _n)	(((_x) + (_n) - 1u) & ~((_n) - 1u))

#define ROUND_DOWN(_x, _n)	((_x) - ((_x) % (_n)))

#define MIN(_x, _y)		((_x) < (_y) ? (_x) : (_y))

#define MAX(_x, _y)		((_x) > (_y) ? (_x) : (_y))

#endif	/* __MATRIX_H__ */
