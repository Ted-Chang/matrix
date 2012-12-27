#ifndef __SYMBOL_H__
#define __SYMBOL_H__

#include "list.h"

/* Structure definition of a symbol */
struct symbol {
	struct list link;	// Link to the list of symbols with this name
	void *addr;		// Address that the symbol points to
	size_t size;		// Size of symbol
	const char *name;	// Name of the symbol
	boolean_t global;	// Whether the symbol is global
	boolean_t exported;	// Whether the symbol has been exported 
};
typedef struct symbol symbol_t;

/* Structure containing a symbol table */
struct symbol_table {
	struct list link;	// Link to symbol table list
	struct symbol *symbols;	// Array of symbols
	size_t count;		// Number of symbols in the table
};
typedef struct symbol_table symbol_table_t;

extern void symbol_table_init(struct symbol_table *t);
extern void symbol_table_destroy(struct symbol_table *t);
extern void symbol_table_insert(struct symbol_table *t, const char *name,
				ptr_t addr, size_t size, boolean_t global,
				boolean_t exported);
extern struct symbol *symbol_lookup_by_addr(ptr_t addr, size_t *offp);
extern struct symbol *symbol_lookup_by_name(const char *name, boolean_t global,
					    boolean_t exported);
extern void init_symbol();

#endif	/* __SYMBOL_H__ */
