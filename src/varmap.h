#ifndef VARMAP_H
#define VARMAP_H

#include <stdbool.h>

#include "token.h"

typedef struct {
    const char* identifier;
    ValueTag type;
} MapItem;

extern MapItem* varmap;
extern size_t varmap_len;

void varmap_add(const char* identifier, ValueTag type);
ValueTag varmap_get(const char* identifier);
bool varmap_key_exists(const char* identifier);
void varmap_free(void);

#endif // VARMAP_H
