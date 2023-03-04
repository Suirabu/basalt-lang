#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "lexer.h"
#include "token.h"

static char peek(const Lexer* lex) {
    return lex->source[lex->sp];
}

static char advance(Lexer* lex) {
    ++lex->column;
    if(peek(lex) == '\n') {
        lex->column = 0;
        ++lex->line;
    }
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

    while(!reached_end(lex) && (isalnum(peek(lex)) || peek(lex) == '_'))
        advance(lex);

    const size_t end = lex->sp;

    const size_t len = end - start;
    char* lexemme = malloc(len + 1);
    memcpy(lexemme, lex->source + start, len);
    lexemme[len] = '\0';
    return (const char*)lexemme;
}

bool collect_string(Lexer* lex, Token* result) {
    size_t line = lex->line;
    size_t column = lex->column;

    advance(lex); // Skip leading double-quote
    const size_t start = lex->sp;

    // TODO: Handle escape sequences ('\n', '\t', etc.)
    while(!reached_end(lex) && peek(lex) != '"')
        advance(lex);

    if(reached_end(lex)) {
        fprintf(stderr, "%s:%lu:%lu: error: expected trailing double-quote, found eof instead\n",
            lex->source_path, line + 1, column + 1
        );
        return false;
    }

    const size_t end = lex->sp;
    advance(lex); // Skip trailing double-quote

    const size_t len = end - start;
    char* string = malloc(len + 1);
    memcpy(string, lex->source + start, len);
    string[len] = '\0';

    Value value = (Value) {
        .tag = VAL_STRING,
        .val_string = string,
    };
    value.global_id = global_add(value);

    *result = (Token) {
        .type = TOK_STRING,
        .value = value,
        .source_path = lex->source_path,
        .line = line,
        .column = column,
    };
    return true;
}

bool collect_number(Lexer* lex, Token* result) {
    size_t line = lex->line;
    size_t column = lex->column;

    // TODO: Validate lexemme before converting to integer
    const char* lexemme = collect_lexemme(lex);
    Value value = (Value) {
        .tag = VAL_INT,
        .val_int = atoi(lexemme),
    };
    free((void*)lexemme);
    *result = (Token) {
        .type = TOK_INT,
        .value = value,
        .source_path = lex->source_path,
        .line = line,
        .column = column,
    };
    return true;
}

bool collect_keyword(Lexer* lex, Token* result) {
    size_t line = lex->line;
    size_t column = lex->column;

    *result = (Token) {
        .source_path = lex->source_path,
        .line = lex->line,
        .column = lex->column,
    };

    const char* lexemme = collect_lexemme(lex);

    if(strcmp(lexemme, "true") == 0) {
        result->type = TOK_BOOL;
        result->value = (Value) {
            .tag = VAL_BOOL,
            .val_bool = true,
        };
    } else if(strcmp(lexemme, "false") == 0) {
        result->type = TOK_BOOL;
        result->value = (Value) {
            .tag = VAL_BOOL,
            .val_bool = false,
        };
    } else if(strcmp(lexemme, "if") == 0) {
        result->type = TOK_IF;
    } else if(strcmp(lexemme, "else") == 0) {
        result->type = TOK_ELSE;
    } else if(strcmp(lexemme, "then") == 0) {
        result->type = TOK_THEN;
    } else if(strcmp(lexemme, "end") == 0) {
        result->type = TOK_END;
    } else if(strcmp(lexemme, "not") == 0) {
        result->type = TOK_NOT;
    } else if(strcmp(lexemme, "var") == 0) {
        result->type = TOK_VAR;
    } else if(strcmp(lexemme, "int") == 0) {
        result->type = TOK_TYPE_INT;
    } else if(strcmp(lexemme, "bool") == 0) {
        result->type = TOK_TYPE_BOOL;
    } else {
        result->type = TOK_IDENTIFIER;
        result->value.tag = VAL_IDENTIFIER;
        result->value.identifier = lexemme;
    }

    return true;
}

bool collect_symbol(Lexer* lex, Token* result) {
    *result = (Token) {
        .source_path = lex->source_path,
        .line = lex->line,
        .column = lex->column,
    };

    const char c = advance(lex);

    switch(c) {
        case '(':
            result->type = TOK_LEFT_PAREN;
            break;
        case ')':
            result->type = TOK_RIGHT_PAREN;
            break;
        case '=':
            if(peek(lex) == '=') {
                advance(lex);
                result->type = TOK_EQUAL_EQUAL;
            } else {
                result->type = TOK_EQUAL;
            }
            break;
        case '!':
            if(peek(lex) != '=') {
                fprintf(stderr, "%s:%lu:%lu: error: expected '=', found '%c' instead\n", lex->source_path, lex->line + 1, lex->column + 1, c);
                return false;
            }
            advance(lex);
            result->type = TOK_BANG_EQUAL;
            break;
        case '<':
            if(peek(lex) == '=') {
                advance(lex);
                result->type = TOK_LESS_EQUAL;
            } else {
                result->type = TOK_LESS;
            }
            break;
        case '>':
            if(peek(lex) == '=') {
                advance(lex);
                result->type = TOK_GREATER_EQUAL;
            } else {
                result->type = TOK_GREATER;
            }
            break;
        case '+':
            if(peek(lex) == '=') {
                advance(lex);
                result->type = TOK_PLUS_EQUAL;
            } else {
                result->type = TOK_PLUS;
            }
            break;
        case '-':
            if(peek(lex) == '=') {
                advance(lex);
                result->type = TOK_MINUS_EQUAL;
            } else {
                result->type = TOK_MINUS;
            }
            break;
        case '*':
            if(peek(lex) == '=') {
                advance(lex);
                result->type = TOK_STAR_EQUAL;
            } else {
                result->type = TOK_STAR;
            }
            break;
        case '/':
            if(peek(lex) == '=') {
                advance(lex);
                result->type = TOK_SLASH_EQUAL;
            } else {
                result->type = TOK_SLASH;
            }
            break;
        case ':':
            result->type = TOK_COLON;
            break;
        case '\0':
            result->type = TOK_EOF;
            break;
        default:
            fprintf(stderr, "%s:%lu:%lu: error: unknown symbol '%c'\n", lex->source_path, lex->line + 1, lex->column + 1, c);
            return false;
    }

    return true;
}

bool lexer_collect_token(Lexer* lex, Token* result) {
    skip_whitespace(lex);

    const char c = peek(lex);
    if(c == '"') {
        return collect_string(lex, result);
    } else if(isdigit(c)) {
        return collect_number(lex, result);
    } else if(isalnum(c)) {
        return collect_keyword(lex, result);
    } else {
        return collect_symbol(lex, result);
    }
}
