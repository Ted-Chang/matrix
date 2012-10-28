#include <types.h>
#include <string.h>	// memset
#include "util.h"
#include "vector.h"
#include "mm/mmu.h"
#include "mm/kmem.h"
#include "matrix/debug.h"

int8_t default_compare(type_t x, type_t y)
{
	if (x == y)
		return 0;
	return (x > y) ? 1 : -1;
}

struct vector *create_vector(uint32_t max_size, compare_t compare)
{
	struct vector *v = NULL;

	return v;
}

void place_vector(struct vector *v, void *addr, uint32_t max_size, compare_t compare)
{
	v->array = (type_t *)addr;
	memset(v->array, 0, max_size * sizeof(type_t));
	v->size = 0;
	v->max_size = max_size;
	v->compare = compare;
}

void destroy_vector(struct vector *vector)
{
}

void insert_vector(struct vector *vector, type_t item)
{
	uint32_t iterator;
	
	ASSERT(vector->compare);

	iterator = 0;
	while ((iterator < vector->size) &&
	       (vector->compare(vector->array[iterator], item) == -1))
		iterator++;
	if (iterator == vector->size) {
		vector->array[iterator] = item;
		vector->size++;
	} else {
		type_t tmp = vector->array[iterator];
		vector->array[iterator] = item;

		while (iterator < vector->size) {
			type_t tmp2;
			iterator++;
			tmp2 = vector->array[iterator];
			vector->array[iterator] = tmp;
			tmp = tmp2;
		}
		vector->size++;
	}
}

type_t lookup_vector(struct vector *vector, uint32_t i)
{
	ASSERT(i < vector->size);
	return vector->array[i];
}

void remove_vector(struct vector *vector, uint32_t i)
{
	ASSERT(i < vector->size);
	while (i < vector->size) {
		vector->array[i] = vector->array[i+1];
		i++;
	}
	vector->size--;
}
