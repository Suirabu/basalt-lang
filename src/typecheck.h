#ifndef TYPECHECK_H
#define TYPECHECK_H

#include <stdbool.h>
#include <stddef.h>

#include "expr.h"

bool typecheck_exprs(Expr** exprs, size_t n_exprs);

#endif // TYPECHECK_H
