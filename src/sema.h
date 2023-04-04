#ifndef SEMA_H
#define SEMA_H

#include <stddef.h>

#include "expr.h"

bool sema_analyze(Expr** exprs, size_t n_exprs);

#endif // SEMA_H
