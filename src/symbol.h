#ifndef SYMBOL_H
#define SYMBOL_H

#include <stdbool.h>
#include <stddef.h>

#include "token.h" // Needed for `ValueTag`

// Structural type of the symbol
typedef enum {
    SYM_VAR,
    SYM_FN,
} StructType;

typedef struct {
    bool exists;
    const char* identifier;
    ValueTag type;
    StructType stype;
    ValueTag* param_types;
    const char** param_identifiers;
    size_t n_params;
    ValueTag return_type;
} Symbol;

extern Symbol* symbol_table;
extern size_t symbol_table_len;

Symbol symbol_add_var(const char* identifier, ValueTag type);
Symbol symbol_add_fn(const char* identifier, ValueTag* param_types, const char** param_identifiers, size_t n_params, ValueTag return_type);
Symbol symbol_get(const char* identifier);
bool symbol_exists(const char* identifier);

#endif // SYMBOL_H
