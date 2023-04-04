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

Expr* expr_create_if(Expr* condition, Expr** if_body, size_t if_body_len, Expr** else_body, size_t else_body_len) {
    Expr* result = malloc(sizeof(Expr));
    result->tag = EXPR_IF;
    result->if_stmt.condition = condition;
    result->if_stmt.if_body = if_body;
    result->if_stmt.if_body_len = if_body_len;
    result->if_stmt.else_body = else_body;
    result->if_stmt.else_body_len = else_body_len;
    return result;
}

Expr* expr_create_var_def(const char* identifier, ValueTag type, Expr* initial_value) {
    Expr* result = malloc(sizeof(Expr));
    result->tag = EXPR_VAR_DEF;
    result->var_def.identifier = identifier;
    result->var_def.type = type;
    result->var_def.initial_value = initial_value;
    return result;    
}

Expr* expr_create_assign(const char* identifier, Token op, Expr* expr) {
    Expr* result = malloc(sizeof(Expr));
    result->tag = EXPR_ASSIGN;
    result->assign.identifier = identifier;
    result->assign.op = op;
    result->assign.expr = expr;
    return result;    
}

Expr* expr_create_while(Expr* condition, Expr** body, size_t body_len) {
    Expr* result = malloc(sizeof(Expr));
    result->tag = EXPR_WHILE;
    result->while_loop.condition = condition;
    result->while_loop.body = body;
    result->while_loop.body_len = body_len;
    return result;    
}

Expr* expr_create_fn_def(const char* identifier, const char** param_identifiers, ValueTag* param_types, size_t n_params, ValueTag return_type, struct _Expr** body, size_t body_len) {
    Expr* result = malloc(sizeof(Expr));
    result->tag = EXPR_FN_DEF;
    result->fn_def.identifier = identifier;
    result->fn_def.param_identifiers = param_identifiers;
    result->fn_def.param_types = param_types;
    result->fn_def.n_params = n_params;
    result->fn_def.return_type = return_type;
    result->fn_def.body = body;
    result->fn_def.body_len = body_len;
    return result;    
}

Expr* expr_create_return(Token op, Expr* value_expr) {
    Expr* result = malloc(sizeof(Expr));
    result->tag = EXPR_RETURN;
    result->op_return.op = op;
    result->op_return.value_expr = value_expr;
    return result;
}

void expr_free(Expr* expr) {
    switch(expr->tag) {
        case EXPR_LITERAL:
            if(expr->literal.value.tag == VAL_STRING)
                free((void*)expr->literal.value.val_string);
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
        case EXPR_IF:
            expr_free(expr->if_stmt.condition);
            for(size_t i = 0; i < expr->if_stmt.if_body_len; ++i)
                expr_free(expr->if_stmt.if_body[i]);
            for(size_t i = 0; i < expr->if_stmt.else_body_len; ++i)
                expr_free(expr->if_stmt.else_body[i]);
            break;
        case EXPR_VAR_DEF:
            if(expr->var_def.initial_value)
                expr_free(expr->var_def.initial_value);
            free((void*)expr->var_def.identifier);
            break;
        case EXPR_ASSIGN:
            expr_free(expr->assign.expr);
            free((void*)expr->assign.identifier);
            break;
        case EXPR_WHILE:
            expr_free(expr->while_loop.condition);
            for(size_t i = 0; i < expr->while_loop.body_len; ++i)
                expr_free(expr->while_loop.body[i]);
            break;
        case EXPR_FN_DEF:
            for(size_t i = 0; i < expr->fn_def.body_len; ++i)
                expr_free(expr->fn_def.body[i]);
            for(size_t i = 0; i < expr->fn_def.n_params; ++i)
                free((void*)expr->fn_def.param_identifiers[i]);
            free((void*)expr->fn_def.identifier);
            free((void*)expr->fn_def.param_types);
            break;
        case EXPR_RETURN:
            break;
    }

    free(expr);
}

#define print_with_indent(...) \
    do { \
        for(size_t i = 0; i < indent; ++i) printf("\033[2m.\033[0m"); \
        printf(__VA_ARGS__); \
    } while(0)

static void token_print(Token tok) {
    printf("%s\n", token_strs[tok.type]);
}

static void token_print_with_indent(Token tok, size_t indent) {
    for(size_t i = 0; i < indent; ++i) {
        printf("\033[2m.\033[0m");
    }
    token_print(tok);
}

static void value_print(Value value) {
    switch(value.tag) {
        case VAL_INT:
            printf("int = %i\n", value.val_int);
            break;
        case VAL_BOOL:
            printf("bool = %s\n", value.val_bool ? "true" : "false");
            break;
        case VAL_STRING:
            printf("string = %s\n", value.val_string);
            break;
        case VAL_IDENTIFIER:
            printf("identifier = %s\n", value.identifier);
            break;
        case VAL_NONE:
            printf("none\n");
            break;
        case VAL_ERROR:
            fprintf(stderr, "error: cannot print erroneous value\n");
            break;
    }
}

static void value_print_with_indent(Value value, size_t indent) {
    for(size_t i = 0; i < indent; ++i) {
        printf("\033[2m.\033[0m");
    }
    value_print(value);
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
        case EXPR_IF:
            print_with_indent("if\n");
            expr_print_with_indent(expr->if_stmt.condition, indent + 1);
            print_with_indent("then\n");
            for(size_t i = 0; i < expr->if_stmt.if_body_len; ++i)
                expr_print_with_indent(expr->if_stmt.if_body[i], indent + 1);
            if(expr->if_stmt.else_body) {
                print_with_indent("else\n");
                for(size_t i = 0; i < expr->if_stmt.else_body_len; ++i)
                    expr_print_with_indent(expr->if_stmt.else_body[i], indent + 1);
            }
            print_with_indent("end\n");
            break;
        case EXPR_VAR_DEF:
            print_with_indent("var %s: %s =\n", expr->var_def.identifier, type_strs[expr->var_def.type]);
            expr_print_with_indent(expr->var_def.initial_value, indent + 1);
            break;
        case EXPR_ASSIGN:
            print_with_indent("%s %s\n", expr->assign.identifier, token_strs[expr->assign.op.type]);
            expr_print_with_indent(expr->assign.expr, indent + 1);
            break;
        case EXPR_WHILE:
            print_with_indent("while\n");
            expr_print_with_indent(expr->while_loop.condition, indent + 1);
            print_with_indent("do\n");
            for(size_t i = 0; i < expr->while_loop.body_len; ++i)
                expr_print_with_indent(expr->while_loop.body[i], indent + 1);
            print_with_indent("end\n");
            break;
        case EXPR_FN_DEF:
            print_with_indent("fn %s (", expr->fn_def.identifier);
            for(size_t i = 0; i < expr->fn_def.n_params; ++i) {
                printf(
                    "%s: %s%s",
                    expr->fn_def.param_identifiers[i],
                    type_strs[expr->fn_def.param_types[i]],
                    i == expr->fn_def.n_params - 1 ? "" : ", "
                );
            }
            printf(") %s\n", type_strs[expr->fn_def.return_type]);
            for(size_t i = 0; i < expr->fn_def.body_len; ++i)
                expr_print_with_indent(expr->fn_def.body[i], indent + 1);
            print_with_indent("end\n");
            break;
        case EXPR_RETURN:
            print_with_indent("return\n");
            expr_print_with_indent(expr->op_return.value_expr, indent + 1);
            break;
    }
}

void expr_print(Expr* expr) {
    expr_print_with_indent(expr, 0);
}
