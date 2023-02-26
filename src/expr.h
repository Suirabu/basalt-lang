#ifndef EXPR_H
#define EXPR_H

#include "token.h"

typedef enum {
    EXPR_LITERAL,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_GROUPING,
} ExprTag;

typedef struct _Expr {
    ExprTag tag;
    union {
        struct { Token tok; } literal;
        struct { Token op; struct _Expr* rhs; } unary;
        struct { struct _Expr* lhs; Token op; struct _Expr* rhs; } binary;
        struct { struct _Expr* expr; } grouping;
    };
} Expr;

Expr* expr_create_literal(Token tok);
Expr* expr_create_unary(Token op, Expr* rhs);
Expr* expr_create_binary(Expr* lhs, Token op, Expr* rhs);
Expr* expr_create_grouping(Expr* expr);
void expr_free(Expr* expr);

void expr_print(Expr* expr);

#endif // EXPR_H
