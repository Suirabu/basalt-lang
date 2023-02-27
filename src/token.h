#ifndef TOKEN_H
#define TOKEN_H

#include <stdbool.h>
#include <stddef.h>

extern const char* token_strs[];
extern const char* type_strs[];

typedef enum {
    VAL_INT,
    VAL_BOOL,
    VAL_ERROR, // Hack needed for type checking
} ValueTag;

typedef struct {
    ValueTag tag;
    union {
        struct { int val_int; };
        struct { bool val_bool; };
    };
} Value;

typedef enum {
    TOK_INT,
    TOK_BOOL,

    TOK_LEFT_PAREN,
    TOK_RIGHT_PAREN,

    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,

    TOK_ERROR,
    TOK_EOF,
} TokenType;

typedef struct {
    TokenType type;
    Value value;
    
    const char* source_path;
    size_t line, column;
} Token;

#endif // TOKEN_H
