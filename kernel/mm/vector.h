#ifndef __VECTOR_H__
#define __VECTOR_H__

typedef void * type_t;

typedef int8_t (*compare_t)(type_t, type_t);

struct vector {
	type_t *array;
	uint32_t size;
	uint32_t max_size;
	compare_t compare;
};

/*
 * A default compare function
 */
int8_t default_compare(type_t x, type_t y);

struct vector *create_vector(uint32_t max_size, compare_t compare);

void place_vector(struct vector *v, void *addr, uint32_t max_size, compare_t compare);
void destroy_vector(struct vector *vector);
void insert_vector(struct vector *vector, type_t item);
type_t lookup_vector(struct vector *vector, uint32_t i);
void remove_vector(struct vector *vector, uint32_t i);

#endif	/* __VECTOR_H__ */
