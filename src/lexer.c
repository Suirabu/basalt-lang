#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "token.h"

static char peek(const Lexer* lex) {
    return lex->source[lex->sp];
}

static char advance(Lexer* lex) {
    ++lex->column;
    return lex->source[lex->sp++];
}

static bool reached_end(const Lexer* lex) {
    return peek(lex) == '\0';
}

static void skip_whitespace(Lexer* lex) {
    while(!reached_end(lex) && isspace(peek(lex)))
        advance(lex);
}

const char* collect_lexemme(Lexer* lex) {
    const size_t start = lex->sp;

    while(!reached_end(lex) && isalnum(peek(lex)))
        advance(lex);

    const size_t end = lex->sp;

    const size_t len = end - start;
    char* lexemme = malloc(len + 1);
    memcpy(lexemme, lex->source + start, len);
    lexemme[len] = '\0';
    return (const char*)lexemme;
}

Token collect_number(Lexer* lex) {
    size_t line = lex->line;
    size_t column = lex->column;

    // TODO: Validate lexemme before converting to integer
    const char* lexemme = collect_lexemme(lex);
    int value = atoi(lexemme);
    free((void*)lexemme);
    return (Token) {
        .type = TOK_INT,
        .value = value,
        .source_path = lex->source_path,
        .line = line,
        .column = column,
    };
}

Token collect_symbol(Lexer* lex) {
    Token tok = (Token) {
        .source_path = lex->source_path,
        .line = lex->line,
        .column = lex->column,
    };

    const char c = advance(lex);

    switch(c) {
        case '(':
            tok.type = TOK_LEFT_PAREN;
            break;
        case ')':
            tok.type = TOK_RIGHT_PAREN;
            break;
        case '+':
            tok.type = TOK_PLUS;
            break;
        case '-':
            tok.type = TOK_MINUS;
            break;
        case '*':
            tok.type = TOK_STAR;
            break;
        case '/':
            tok.type = TOK_SLASH;
            break;
        case '\0':
            tok.type = TOK_EOF;
            break;
        default:
            fprintf(stderr, "%s:%lu:%lu: error: unknown symbol '%c'\n", lex->source_path, lex->line + 1, lex->column + 1, c);
            tok.type = TOK_ERROR;
            break;
    }

    return tok;
}

Token lexer_collect_token(Lexer* lex) {
    skip_whitespace(lex);

    const char c = peek(lex);
    if(isalnum(c)) {
        return collect_number(lex);
    } else {
        return collect_symbol(lex);
    }
}
