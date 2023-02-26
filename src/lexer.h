#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>
#include <stddef.h>

#include "token.h"

typedef struct {
    const char* source;
    size_t sp;

    const char* source_path;
    size_t line, column;
} Lexer;

bool lexer_collect_token(Lexer* lex, Token* result);

#endif // LEXER_H
