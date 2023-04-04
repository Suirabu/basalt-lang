#include <stdbool.h>
#include <stdio.h>

#include "expr.h"
#include "sema.h"

bool sema_fn(Expr* expr) {
    if(expr->fn_def.body[expr->fn_def.body_len - 1]->tag != EXPR_RETURN) {
        fprintf(stderr, "error: function `%s` must return value\n", expr->fn_def.identifier);
        return false;
    }
    return true;
}

bool sema_analyze(Expr **exprs, size_t n_exprs) {
    for(size_t i = 0; i < n_exprs; ++i) {
        switch(exprs[i]->tag) {
            case EXPR_FN_DEF:
                if(!sema_fn(exprs[i]))
                    return false;
            default:
                break;
        }
    }
    return true;
}
