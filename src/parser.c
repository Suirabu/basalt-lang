#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "expr.h"
#include "parser.h"
#include "token.h"

static Token peek(const Parser* par) {
    return par->tokens[par->tp];
}

static Token previous(const Parser* par) {
    return par->tokens[par->tp - 1];
}

static Token advance(Parser* par) {
    return par->tokens[par->tp++];
}

static bool check(Parser* par, TokenType type) {
    return peek(par).type == type;
}

static bool match(Parser* par, TokenType type) {
    if(check(par, type)) {
        advance(par);
        return true;
    }

    return false;
}

static bool expect(Parser* par, TokenType type) {
    if(!match(par, type)) {
        const Token tok = peek(par);
        fprintf(stderr, "%s:%lu:%lu: error: expected %s, found %s instead\n",
            tok.source_path, tok.line + 1, tok.column + 1,
            token_strs[type], token_strs[tok.type]
        );
        return false;
    }

    return true;
}

Expr* collect_primary(Parser* par) {
    if(match(par, TOK_INT) || match(par, TOK_BOOL) || match(par, TOK_STRING)) {
        return expr_create_literal(previous(par).value);
    }

    if(match(par, TOK_LEFT_PAREN)) {
        Expr* expr = parser_collect_expr(par);
        if(!expect(par, TOK_RIGHT_PAREN))
            return NULL;
        return expr_create_grouping(expr);
    }

    const Token tok = peek(par);
    fprintf(stderr, "%s:%lu:%lu: error: failed to parse primary expr from %s\n",
        tok.source_path, tok.line + 1, tok.column + 1,
        token_strs[tok.type]
    );
    return NULL;
}

Expr* collect_unary(Parser* par) {
    if(match(par, TOK_MINUS)) {
        Token op = previous(par);
        Expr* rhs = collect_unary(par);
        return expr_create_unary(op, rhs);
    }

    return collect_primary(par);
}

Expr* collect_factor(Parser* par) {
    Expr* expr = collect_unary(par);

    while(match(par, TOK_STAR) || match(par, TOK_SLASH)) {
        Token op = previous(par);
        Expr* rhs = collect_unary(par);
        expr = expr_create_binary(expr, op, rhs);
    }

    return expr;
}

Expr* collect_term(Parser* par) {
    Expr* expr = collect_factor(par);

    while(match(par, TOK_PLUS) || match(par, TOK_MINUS)) {
        Token op = previous(par);
        Expr* rhs = collect_factor(par);
        expr = expr_create_binary(expr, op, rhs);
    }

    return expr;
}

bool parser_reached_end(const Parser* par) {
    return peek(par).type == TOK_EOF;
}

Expr* parser_collect_expr(Parser* par) {
    return collect_term(par);
}
