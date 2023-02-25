#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

#include "token.h"

typedef struct {
    const char* source;
    size_t sp;

    const char* source_path;
    size_t line, column;
} Lexer;

Token lexer_collect_token(Lexer* lex);

#endif // LEXER_H
