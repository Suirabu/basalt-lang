#ifndef EXPR_H
#define EXPR_H

#include "token.h"

typedef enum {
    EXPR_LITERAL,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_GROUPING,
    EXPR_IF,
    EXPR_VAR_DEF,
    EXPR_ASSIGN,
    EXPR_WHILE,
    EXPR_FN_DEF,
    EXPR_RETURN,
} ExprTag;

typedef struct _Expr {
    ExprTag tag;
    union {
        struct { Value value; } literal;
        struct { Token op; struct _Expr* rhs; } unary;
        struct { struct _Expr* lhs; Token op; struct _Expr* rhs; } binary;
        struct { struct _Expr* expr; } grouping;
        struct {
            struct _Expr* condition;
            struct _Expr** if_body;
            size_t if_body_len;
            struct _Expr** else_body;
            size_t else_body_len;
        } if_stmt;
        struct {
            const char* identifier;
            ValueTag type;
            struct _Expr* initial_value;
        } var_def;
        struct {
            const char* identifier;
            Token op;
            struct _Expr* expr;
        } assign;
        struct {
            struct _Expr* condition;
            struct _Expr** body;
            size_t body_len;
        } while_loop;
        struct {
            const char* identifier;
            const char** param_identifiers;
            ValueTag* param_types;
            size_t n_params;
            ValueTag return_type;
            struct _Expr** body;
            size_t body_len;
        } fn_def;
        struct {
            Token op;
            struct _Expr* value_expr;
        } op_return;
    };
} Expr;

Expr* expr_create_literal(Value value);
Expr* expr_create_unary(Token op, Expr* rhs);
Expr* expr_create_binary(Expr* lhs, Token op, Expr* rhs);
Expr* expr_create_grouping(Expr* expr);
Expr* expr_create_if(Expr* condition, Expr** if_body, size_t if_body_len, Expr** else_body, size_t else_body_len);
Expr* expr_create_var_def(const char* identifier, ValueTag type, Expr* initial_value);
Expr* expr_create_assign(const char* identifier, Token op, Expr* expr);
Expr* expr_create_while(Expr* condition, Expr** body, size_t body_len);
Expr* expr_create_fn_def(const char* identifier, const char** param_identifiers, ValueTag* param_types, size_t n_params, ValueTag return_type, struct _Expr** body, size_t body_len);
Expr* expr_create_return(Token op, Expr* value_expr);
void expr_free(Expr* expr);

void expr_print(Expr* expr);

#endif // EXPR_H
