#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdbool.h>

#include "token.h"

extern Value* global_values;
extern size_t n_global_values;

size_t global_add(Value value);
Value* global_get(size_t id);
void global_free_all(void);

#endif // GLOBAL_H
