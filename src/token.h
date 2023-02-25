#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

extern const char* token_strs[];

typedef enum {
    TOK_INT,

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
    int value;
    
    const char* source_path;
    size_t line, column;
} Token;

#endif // TOKEN_H
