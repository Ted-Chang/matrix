#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "mm/malloc.h"
#include "symbol.h"

extern struct symbol_table _kernel_symtab;

/* List of all symbol tables */
static struct list _symbol_tables = {
	.prev = &_symbol_tables,
	.next = &_symbol_tables
};

void symbol_table_init(struct symbol_table *t)
{
	LIST_INIT(&t->link);
	t->symbols = NULL;
	t->count = 0;
}

void symbol_table_destroy(struct symbol_table *t)
{
	size_t i;

	if (t->symbols) {
		if (!LIST_EMPTY(&t->link)) {
			for (i = 0; i < t->count; i++) {
				/* Remove the symbol of this symbol table */
				;
			}
		}
		kfree(t->symbols);
	}
	
	list_del(&t->link);
}

void symbol_table_insert(struct symbol_table *t, const char *name,
			 ptr_t addr, size_t size, boolean_t global,
			 boolean_t exported)
{
	;
}

struct symbol *symbol_table_lookup_by_addr(struct symbol_table *t,
					   ptr_t addr, size_t *offp)
{
	size_t i;
	
	if (addr < (ptr_t)t->symbols[0].addr) {
		return NULL;
	}

	for (i = 0; i < t->count; i++) {
		;
	}

	if (offp != NULL) {
		*offp = 0;
	}

	return NULL;
}

struct symbol *symbol_table_lookup_by_name(struct symbol_table *t,
					   ptr_t addr, size_t *offp)
{
	return NULL;
}

struct symbol *symbol_lookup_by_addr(ptr_t addr, size_t *offp)
{
	if (!LIST_EMPTY(&_symbol_tables)) {
		return NULL;
	} else {
		return NULL;
	}
}

struct symbol *symbol_lookup_by_name(const char *name, boolean_t global,
				     boolean_t exported)
{
	return NULL;
}

void init_symbol()
{
	;
}
