#include <stdlib.h>
#include <stdio.h>

#include "expr.h"
#include "token.h"

Expr* expr_create_literal(Value value) {
    Expr* result = malloc(sizeof(Expr));
    result->tag = EXPR_LITERAL;
    result->literal.value = value;
    return result;
}

Expr* expr_create_unary(Token op, Expr* rhs) {
    Expr* result = malloc(sizeof(Expr));
    result->tag = EXPR_UNARY;
    result->unary.op = op;
    result->unary.rhs = rhs;
    return result;
}

Expr* expr_create_binary(Expr* lhs, Token op, Expr* rhs) {
    Expr* result = malloc(sizeof(Expr));
    result->tag = EXPR_BINARY;
    result->binary.lhs = lhs;
    result->binary.op = op;
    result->binary.rhs = rhs;
    return result;
}

Expr* expr_create_grouping(Expr* expr) {
    Expr* result = malloc(sizeof(Expr));
    result->tag = EXPR_GROUPING;
    result->grouping.expr = expr;
    return result;
}

void expr_free(Expr* expr) {
    switch(expr->tag) {
        case EXPR_LITERAL:
            break;
        case EXPR_UNARY:
            expr_free(expr->unary.rhs);
            break;
        case EXPR_BINARY:
            expr_free(expr->binary.lhs);
            expr_free(expr->binary.rhs);
            break;
        case EXPR_GROUPING:
            expr_free(expr->grouping.expr);
            break;
    }

    free(expr);
}

static void token_print_with_indent(Token tok, size_t indent) {
    for(size_t i = 0; i < indent; ++i) {
        printf("\033[2m.\033[0m");
    }
    printf("%s\n", token_strs[tok.type]);
}

static void value_print_with_indent(Value value, size_t indent) {
    for(size_t i = 0; i < indent; ++i) {
        printf("\033[2m.\033[0m");
    }
    if(value.tag == VAL_INT) {
        printf("int = %i\n", value.val_int);
    } else if(value.tag == VAL_BOOL) {
        printf("bool = %s\n", value.val_bool ? "true" : "false");
    }
}

static void expr_print_with_indent(Expr* expr, size_t indent) {
    switch(expr->tag) {
        case EXPR_LITERAL:
            value_print_with_indent(expr->literal.value, indent);
            break;
        case EXPR_UNARY:
            token_print_with_indent(expr->unary.op, indent);
            expr_print_with_indent(expr->unary.rhs, indent + 1);
            break;
        case EXPR_BINARY:
            expr_print_with_indent(expr->binary.lhs, indent + 1);
            token_print_with_indent(expr->binary.op, indent);
            expr_print_with_indent(expr->binary.rhs, indent + 1);
            break;
        case EXPR_GROUPING:
            expr_print_with_indent(expr->grouping.expr, indent + 1);
            break;
    }
}

void expr_print(Expr* expr) {
    expr_print_with_indent(expr, 0);
}
