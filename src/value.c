#include <stdlib.h>
#include <stdio.h>

#include "symbol.h"
#include "value.h"

const char* type_strs[] = {
    "int",
    "bool",
    "string",
    "identifier",
    "none",
    "error",
};

size_t get_type_size(ValueTag type) {
    switch(type) {
        case VAL_INT:
            return SIZE_INT;
        case VAL_BOOL:
            return SIZE_BOOL;
        case VAL_STRING:
            return SIZE_STRING;

        default:
            fprintf(stderr, "error: cannot get size of type %s\n", type_strs[type]);
            exit(1);
    }
}
