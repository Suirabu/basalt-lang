#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "expr.h"
#include "parser.h"
#include "token.h"
#include "varmap.h"

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

static bool check_type(Parser* par) {
    const Token t = peek(par);

    if(t.type != TOK_TYPE_INT && t.type != TOK_TYPE_BOOL) {
        const Token tok = peek(par);
        fprintf(stderr, "%s:%lu:%lu: error: expected type, found %s instead\n",
            tok.source_path, tok.line + 1, tok.column + 1,
            token_strs[tok.type]
        );
        return false;
    }

    return true;
}

Expr* collect_primary(Parser* par) {
    if(match(par, TOK_INT) || match(par, TOK_BOOL) || match(par, TOK_STRING)) {
        return expr_create_literal(previous(par).value);
    }

    // Variable
    if(match(par, TOK_IDENTIFIER)) {
        const Token iden = previous(par);
        if(!varmap_key_exists(iden.value.identifier)) {
            fprintf(stderr, "%s:%lu:%lu: error: use of undefined variable %s\n",
                iden.source_path, iden.line + 1, iden.column + 1,
                iden.value.identifier
            );
            return NULL;
        }

        return expr_create_literal(iden.value);
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

Expr* collect_assignment(Parser* par) {
    Expr* expr = collect_equality(par);

    if(match(par, TOK_EQUAL)) {
        Token op = previous(par);
        Expr* rhs = parser_collect_expr(par);

        if(expr->tag != EXPR_LITERAL || expr->literal.value.tag != VAL_IDENTIFIER) {
            fprintf(stderr, "%s:%lu:%lu: error: invalid assignment target\n", op.source_path, op.line + 1, op.column + 1);
            return NULL;
        }

        const char* identifier = expr->literal.value.identifier;

        if(!varmap_key_exists(identifier)) {
            fprintf(stderr, "%s:%lu:%lu: error: cannot assign non-existant variable %s\n",
                op.source_path, op.line + 1, op.column + 1, identifier
            );
            return NULL;
        }

        return expr_create_assign(identifier, op, rhs);
    }

    return expr;
}

Expr* collect_if(Parser* par) {
    Expr* condition = parser_collect_expr(par);
    if(!expect(par, TOK_THEN))
        return NULL;
    
    Expr** if_body = NULL;
    size_t if_body_len = 0;
    while(!(match(par, TOK_END) || match(par, TOK_ELSE))) {
        Expr* expr = parser_collect_expr(par);
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
        Expr* expr = parser_collect_expr(par);
        if(!expr)
            return NULL;
        else_body = realloc(else_body, sizeof(Expr*) * (else_body_len + 1));
        else_body[else_body_len++] = expr;
    }

    return expr_create_if(condition, if_body, if_body_len, else_body, else_body_len);
}

static Expr* collect_var_definition(Parser* par) {
    Token start = previous(par);

    // Get identifier
    if(!expect(par, TOK_IDENTIFIER))
        return NULL;
    Token identifier = previous(par);

    if(!expect(par, TOK_COLON))
        return NULL;
    
    if(!check_type(par))
        return NULL;
    
    Token type = advance(par);
    ValueTag value_tag = token_type_to_value_tag(type.type);

    // Get optional variable initializer
    Expr* initializer = NULL;

    if(match(par, TOK_EQUAL)) {
        initializer = parser_collect_expr(par);
    }

    if(varmap_key_exists(identifier.value.identifier)) {
        fprintf(stderr, "%s:%lu:%lu: error: redefinition of variable %s\n",
            start.source_path, start.line + 1, start.column + 1,
            identifier.value.identifier
        );
        return NULL;
    }
    varmap_add(identifier.value.identifier, value_tag);
    return expr_create_var_def(identifier.value.identifier, value_tag, initializer);
}

Expr* collect_statement(Parser* par) {
    if(match(par, TOK_IF)) {
        return collect_if(par);
    } else if(match(par, TOK_VAR)) {
        return collect_var_definition(par);
    } else {
        // TODO: Exprs should not exist on their own
        return collect_assignment(par);
    }
}

bool parser_reached_end(const Parser* par) {
    return peek(par).type == TOK_EOF;
}

Expr* parser_collect_expr(Parser* par) {
    return collect_statement(par);
}
