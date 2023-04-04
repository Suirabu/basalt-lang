#ifndef VARMAP_H
#define VARMAP_H

#include <stdbool.h>
#include <stddef.h>

#include "token.h"

typedef enum {
    MAP_VAR,
    MAP_FN,
} MapItemTag;

typedef struct {
    const char* identifier;
    MapItemTag tag;
    union {
        struct {
            ValueTag* param_types;
            size_t n_params;
            ValueTag return_type;
        } fn_signature;
        ValueTag type;
    };
} MapItem;

extern MapItem* varmap;
extern size_t varmap_len;

void varmap_add_var(const char* identifier, ValueTag type);
void varmap_add_fn(const char* identifier, ValueTag* param_types, size_t n_params, ValueTag return_type);
MapItem* varmap_get(const char* identifier);
MapItem* varmap_get_var(const char* identifier);
MapItem* varmap_get_fn(const char* identifier);
bool varmap_key_exists(const char* identifier);
void varmap_free(void);

#endif // VARMAP_H
