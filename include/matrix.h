#ifndef __MATRIX_H__
#define __MATRIX_H__

#ifndef INLINE
#define INLINE	inline
#endif

#define FLAG_ON(_x, _f)		((_x) & (_f))

#define SET_FLAG(_x, _f)	((_x) |= (_f))

#define CLEAR_FLAG(_x, _f)	((_x) &= ~(_f))

#endif	/* __MATRIX_H__ */
