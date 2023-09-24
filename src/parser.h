#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stddef.h>

#include "expr.h"
#include "token.h"

extern Symbol* parent_fn;

typedef struct {
    const Token* tokens;
    size_t tp;
} Parser;

bool parser_reached_end(const Parser* par);
Expr* parser_collect_expr(Parser* par);

#endif // PARSER_H
