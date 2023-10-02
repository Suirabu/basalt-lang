#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "symbol.h"
#include "token.h"

Symbol* symbol_table = NULL;
size_t symbol_table_len = 0;

Symbol symbol_add_var(const char* identifier, ValueTag type) {
    ++symbol_table_len;
    symbol_table = realloc(symbol_table, sizeof(Symbol) * symbol_table_len);
    symbol_table[symbol_table_len - 1] = (Symbol) {
        .exists = true,
        .identifier = identifier,
        .type = type,
        .stype = SYM_VAR,
    };
    return symbol_table[symbol_table_len - 1];
}

Symbol symbol_add_fn(const char* identifier, ValueTag* param_types, const char** param_identifiers, size_t n_params, ValueTag return_type) {
    ++symbol_table_len;
    symbol_table = realloc(symbol_table, sizeof(Symbol) * symbol_table_len);
    symbol_table[symbol_table_len - 1] = (Symbol) {
        .exists = true,
        .identifier = identifier,
        .param_types = param_types,
        .param_identifiers = param_identifiers,
        .n_params = n_params,
        .return_type = return_type,
        .stype = SYM_FN,
    };
    return symbol_table[symbol_table_len - 1];
}

Symbol symbol_get(const char* identifier) {
    for(size_t i = 0; i < symbol_table_len; ++i) {
        if(strcmp(identifier, symbol_table[i].identifier) == 0)
            return symbol_table[i];
    }
    return (Symbol) { .exists = false };
}

bool symbol_exists(const char* identifier) {
    return symbol_get(identifier).exists;
}
