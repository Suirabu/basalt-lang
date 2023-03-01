#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "expr.h"
#include "token.h"
#include "typecheck.h"

#define REPORT_ERROR(op, ...) \
    fprintf(stderr, "%s:%lu:%lu: error: ", op.source_path, op.line + 1, op.column + 1); \
    fprintf(stderr, __VA_ARGS__)

static ValueTag get_expr_value(Expr* expr);

static ValueTag get_unary_value(Expr* expr) {
    fprintf(stderr, "error: typechecking unary values is unimplemented\n");
    return VAL_ERROR;
}

static ValueTag get_binary_value(Expr* expr) {
    const ValueTag lhs = get_expr_value(expr->binary.lhs);
    if(lhs == VAL_ERROR)
        return VAL_ERROR;
    const ValueTag rhs = get_expr_value(expr->binary.rhs);
    if(rhs == VAL_ERROR)
        return VAL_ERROR;
    const Token op = expr->binary.op;

    switch(op.type) {
        case TOK_PLUS:
        case TOK_MINUS:
        case TOK_STAR:
        case TOK_SLASH:
            if(lhs != VAL_INT || rhs != VAL_INT) {
                REPORT_ERROR(op, "cannot perform %s operation on types %s and %s\n", token_strs[op.type], type_strs[lhs], type_strs[rhs]);
                return VAL_ERROR;
            }
            return VAL_INT;

        case TOK_EQUAL_EQUAL:
        case TOK_BANG_EQUAL:
            if(lhs != rhs) {
                REPORT_ERROR(op, "cannot perform %s operation on types %s and %s\n", token_strs[op.type], type_strs[lhs], type_strs[rhs]);
                return VAL_ERROR;
            }
            return VAL_BOOL;

        case TOK_LESS:
        case TOK_LESS_EQUAL:
        case TOK_GREATER:
        case TOK_GREATER_EQUAL:
            if(lhs != VAL_INT || rhs != VAL_INT) {
                REPORT_ERROR(op, "cannot perform %s operation on types %s and %s\n", token_strs[op.type], type_strs[lhs], type_strs[rhs]);
                return VAL_ERROR;
            }
            return VAL_BOOL;

        default:
            return VAL_ERROR;
    }
}

static ValueTag get_if_value(Expr* expr) {
    if(get_expr_value(expr->if_stmt.condition) == VAL_ERROR)
        return VAL_ERROR;

    for(size_t i = 0; i < expr->if_stmt.if_body_len; ++i)
        if(get_expr_value(expr->if_stmt.if_body[i]) == VAL_ERROR)
            return VAL_ERROR;
    for(size_t i = 0; i < expr->if_stmt.else_body_len; ++i)
        if(get_expr_value(expr->if_stmt.else_body[i]) == VAL_ERROR)
            return VAL_ERROR;
    return VAL_NONE;
}

static ValueTag get_expr_value(Expr* expr) {
    switch(expr->tag) {
        case EXPR_LITERAL:
            return expr->literal.value.tag;
        case EXPR_UNARY:
            return get_unary_value(expr);
        case EXPR_BINARY:
            return get_binary_value(expr);
        case EXPR_GROUPING:
            return get_expr_value(expr->grouping.expr);
        case EXPR_IF:
            return get_if_value(expr);
    }
}

bool typecheck_exprs(Expr** exprs, size_t n_exprs) {
    bool has_error = false;

    for(size_t i = 0; i < n_exprs; ++i) {
        if(get_expr_value(exprs[i]) == VAL_ERROR)
            has_error = true;
    }

    return !has_error;
}
