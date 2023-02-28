#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "token.h"

Value* global_values = NULL;
size_t n_global_values = 0;

size_t global_add(Value value) {
    value.global_id = n_global_values;
    global_values = realloc(global_values, sizeof(Value) * (n_global_values + 1));
    global_values[n_global_values] = value;
    return n_global_values++;
}

Value* global_get(size_t id) {
    return &global_values[id];
}

void global_free_all(void) {
    free(global_values);
}
