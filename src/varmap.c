#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "token.h"
#include "varmap.h"

MapItem* varmap = NULL;
size_t varmap_len = 0;

void varmap_add(const char* identifier, ValueTag type) {
    varmap = realloc(varmap, sizeof(MapItem) * ++varmap_len);
    varmap[varmap_len - 1] = (MapItem) {
        .identifier = identifier,
        .type = type,
    };
}

ValueTag varmap_get(const char* identifier) {
    for(size_t i = 0; i < varmap_len; ++i) {
        if(strcmp(identifier, varmap[i].identifier) == 0)
            return varmap[i].type;
    }

    return VAL_ERROR;
}

bool varmap_key_exists(const char* identifier) {
    return varmap_get(identifier) != VAL_ERROR;
}

void varmap_free(void) {
    free(varmap);
    varmap = NULL;
    varmap_len = 0;
}
