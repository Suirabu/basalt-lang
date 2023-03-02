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
    exit(1);
    return NULL;
}

Expr* collect_unary(Parser* par) {
    if(match(par, TOK_MINUS) || match(par, TOK_NOT)) {
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

Expr* collect_comparison(Parser* par) {
    Expr* expr = collect_term(par);

    while(match(par, TOK_LESS)
          || match(par, TOK_LESS_EQUAL)
          || match(par, TOK_GREATER)
          || match(par, TOK_GREATER_EQUAL)
    ) {
        Token op = previous(par);
        Expr* rhs = collect_unary(par);
        expr = expr_create_binary(expr, op, rhs);
    }

    return expr;
}

Expr* collect_equality(Parser* par) {
    Expr* expr = collect_comparison(par);

    while(match(par, TOK_EQUAL_EQUAL) || match(par, TOK_BANG_EQUAL)) {
        Token op = previous(par);
        Expr* rhs = collect_unary(par);
        expr = expr_create_binary(expr, op, rhs);
    }

    return expr;
}

Expr* collect_expr(Parser* par) {
    return collect_equality(par);
}

Expr* collect_if(Parser* par) {
    Expr* condition = collect_expr(par);
    if(!expect(par, TOK_THEN))
        return NULL;
    
    Expr** if_body = NULL;
    size_t if_body_len = 0;
    while(!(match(par, TOK_END) || match(par, TOK_ELSE))) {
        Expr* expr = collect_expr(par);
        if(!expr)
            return NULL;
        if_body = realloc(if_body, sizeof(Expr*) * (if_body_len + 1));
        if_body[if_body_len++] = expr;
    }

    if(previous(par).type == TOK_END) {
        return expr_create_if(condition, if_body, if_body_len, NULL, 0);
    }

    if(previous(par).type != TOK_ELSE) {
        Token prev = previous(par);
        fprintf(stderr, "%s:%lu:%lu: error: expected end or else, found %s instead\n",
            prev.source_path, prev.line + 1, prev.column + 1,
            token_strs[prev.type]
        );
        return NULL;
    }

    Expr** else_body = NULL;
    size_t else_body_len = 0;
    while(!match(par, TOK_END)) {
        Expr* expr = collect_expr(par);
        if(!expr)
            return NULL;
        else_body = realloc(else_body, sizeof(Expr*) * (else_body_len + 1));
        else_body[else_body_len++] = expr;
    }

    return expr_create_if(condition, if_body, if_body_len, else_body, else_body_len);
}

Expr* collect_statement(Parser* par) {
    if(match(par, TOK_IF)) {
        return collect_if(par);
    } else {
        // TODO: Exprs should not exist on their own
        return collect_expr(par);
    }
}

bool parser_reached_end(const Parser* par) {
    return peek(par).type == TOK_EOF;
}

Expr* parser_collect_expr(Parser* par) {
    return collect_statement(par);
}
