#ifndef TOKEN_H
#define TOKEN_H

#include <stdbool.h>
#include <stddef.h>

extern const char* token_strs[];
extern const char* type_strs[];

typedef enum {
    VAL_INT,
    VAL_BOOL,
    VAL_STRING,
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
    };
} Value;

typedef enum {
    TOK_INT,
    TOK_BOOL,
    TOK_STRING,

    TOK_LEFT_PAREN,
    TOK_RIGHT_PAREN,

    TOK_EQUAL_EQUAL,
    TOK_BANG_EQUAL,
    TOK_LESS,
    TOK_LESS_EQUAL,
    TOK_GREATER,
    TOK_GREATER_EQUAL,

    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,

    TOK_IF,
    TOK_ELSE,
    TOK_THEN,
    TOK_END,
    TOK_NOT,

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
