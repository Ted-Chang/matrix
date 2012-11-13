#include <types.h>
#include <stddef.h>
#include "matrix/matrix.h"
#include "symbol.h"

extern struct symbol_table _kernel_symtab;

/* List of all symbol tables */
static struct list _symbol_tables = {
	.prev = &_symbol_tables,
	.next = &_symbol_tables
};

void init_symbol()
{
	;
}
