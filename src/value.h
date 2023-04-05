#ifndef VALUE_H
#define VALUE_H

#include <stdbool.h>
#include <stddef.h>

extern const char* type_strs[];

#define SIZE_INT 8
#define SIZE_BOOL 1
#define SIZE_STRING 8

typedef enum {
    VAL_INT,
    VAL_BOOL,
    VAL_STRING,
    VAL_IDENTIFIER,
    VAL_NONE,
    VAL_ERROR, // Hack needed for type checking
} ValueTag;

typedef struct {
    ValueTag tag;
    size_t global_id;
    union {
        struct { int val_int; };
        struct { bool val_bool; };
        struct { const char* val_string; };
        struct { const char* identifier; };
    };
} Value;

size_t get_type_size(ValueTag type);

#endif // VALUE_H
