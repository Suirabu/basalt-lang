#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>

#include "codegen.h"
#include "expr.h"
#include "global.h"
#include "token.h"

static const char* registers[] = {
    "r9", "r10", "r11", "r12", "r13", "r14", "r15",
};
#define n_registers (sizeof(registers) / sizeof(char*))

static int free_registers[n_registers];
static size_t n_free_registers = n_registers;

static void initialize_registers(void) {
    for(size_t i = 0; i < n_free_registers; ++i) {
        free_registers[i] = i;
    }
}

static int allocate_register(void) {
    if(n_free_registers == 0) {
        fprintf(stderr, "error: failed to allocate register\n");
        return -1;
    }

    return free_registers[--n_free_registers];
}

static void free_register(int reg) {
    free_registers[n_free_registers++] = reg;
}

static int write_assembly_for_expr(Expr* expr, FILE* out);

static void write_globals(FILE* out) {
    fprintf(out, "section .bss\n");

    for(size_t i = 0; i < n_global_values; ++i) {
        // For now we can assume that all global values are strings, but this won't be the case for long
        // TODO: Switch on value tag instead of assuming all globals will be strings
        fprintf(out,
            "    str_%lu: db \"%s\", 0\n",
            global_values[i].global_id,
            global_values[i].val_string
        );
    }

    fprintf(out, "\n");
}

static void write_preamble(FILE* out) {
    fprintf(out,
        "section .text\n"
        "global _start\n\n"
        "_start:\n"
    );
}

static int write_literal(Expr* expr, FILE* out) {
    const int reg = allocate_register();
    switch(expr->literal.value.tag) {
        case VAL_INT:
            fprintf(out, "    mov %s, %i\n", registers[reg], expr->literal.value.val_int);
            break;
        case VAL_BOOL:
            fprintf(out, "    mov %s, %i\n", registers[reg], expr->literal.value.val_bool);
            break;
        case VAL_STRING:
            fprintf(out, "    mov %s, str_%lu\n", registers[reg], expr->literal.value.global_id);
            break;
        case VAL_ERROR:
            fprintf(stderr, "error: cannot generate code for erroneous value\n");
            return -1;
    }

    return reg;
}

static int write_unary(Expr* expr, FILE* out) {
    fprintf(stderr, "error: unary expressions are unimplemented\n");
    return -1;
}

static int write_binary(Expr* expr, FILE* out) {
    const int lhs_reg = write_assembly_for_expr(expr->binary.lhs, out);
    const int rhs_reg = write_assembly_for_expr(expr->binary.rhs, out);

    switch(expr->binary.op.type) {
        case TOK_PLUS:
            fprintf(out, "    add %s, %s\n",
                registers[lhs_reg],
                registers[rhs_reg]
            );
            break;
        case TOK_MINUS:
            fprintf(out, "    sub %s, %s\n",
                registers[lhs_reg],
                registers[rhs_reg]
            );
            break;
        case TOK_STAR:
            fprintf(out, "    imul %s, %s\n",
                registers[lhs_reg],
                registers[rhs_reg]
            );
            break;
        case TOK_SLASH:
            fprintf(out, 
                "    mov rax, %s\n"
                "    xor rdx, rdx\n"
                "    idiv %s\n"
                "    mov %s, rax\n",
                registers[lhs_reg],
                registers[rhs_reg],
                registers[lhs_reg]
            );
            break;
        default:
            fprintf(stderr, "error: unknown binary operation '%s'\n", token_strs[expr->binary.op.type]);
            return -1;
    }

    free_register(rhs_reg);
    return lhs_reg;
}

static int write_grouping(Expr* expr, FILE* out) {
    return write_assembly_for_expr(expr->grouping.expr, out);
}

static int write_assembly_for_expr(Expr* expr, FILE* out) {
    switch(expr->tag) {
        case EXPR_LITERAL:
            return write_literal(expr, out);
        case EXPR_UNARY:
            return write_unary(expr, out);
        case EXPR_BINARY:
            return write_binary(expr, out);
        case EXPR_GROUPING:
            return write_grouping(expr, out);
    }
}

bool generate_assembly(Expr** exprs, size_t n_exprs, const char* output_path) {
    initialize_registers();

    FILE* output_file = fopen(output_path, "w");
    if(!output_file) {
        fprintf(stderr, "error: failed to open file '%s' for writing\n", output_path);
        return false;
    }

    write_globals(output_file);
    write_preamble(output_file);

    for(size_t i = 0; i < n_exprs; ++i) {
        const int reg = write_assembly_for_expr(exprs[i], output_file);
        free_register(reg); // We won't be needing this register for now
    }

    fclose(output_file);
    return true;
}
