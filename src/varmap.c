#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "token.h"
#include "varmap.h"

MapItem* varmap = NULL;
size_t varmap_len = 0;

void varmap_add_var(const char* identifier, ValueTag type) {
    varmap = realloc(varmap, sizeof(MapItem) * ++varmap_len);
    varmap[varmap_len - 1] = (MapItem) {
        .identifier = identifier,
        .tag = MAP_VAR,
        .type = type,
    };
}

void varmap_add_fn(const char* identifier, ValueTag* param_types, size_t n_params, ValueTag return_type) {
    varmap = realloc(varmap, sizeof(MapItem) * ++varmap_len);
    varmap[varmap_len - 1] = (MapItem) {
        .identifier = identifier,
        .tag = MAP_FN,
        .fn_signature.param_types = param_types,
        .fn_signature.n_params = n_params,
        .fn_signature.return_type = return_type,
    };
}

MapItem* varmap_get(const char* identifier) {
    for(size_t i = 0; i < varmap_len; ++i) {
        if(strcmp(identifier, varmap[i].identifier) == 0)
            return &varmap[i];
    }

    return NULL;
}

MapItem* varmap_get_var(const char* identifier) {
    MapItem* item = varmap_get(identifier);
    if(!item || item->tag != MAP_VAR)
        return NULL;
    return item;
}

MapItem* varmap_get_fn(const char* identifier) {
    MapItem* item = varmap_get(identifier);
    if(!item || item->tag != MAP_FN)
        return NULL;
    return item;
}

bool varmap_key_exists(const char* identifier) {
    return varmap_get(identifier) != NULL;
}

void varmap_free(void) {
    free(varmap);
    varmap = NULL;
    varmap_len = 0;
}
