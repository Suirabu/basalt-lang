#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdbool.h>
#include <stddef.h>

#include "expr.h"

bool generate_assembly(Expr** exprs, size_t n_exprs, const char* output_path);

#endif // CODEGEN_H
