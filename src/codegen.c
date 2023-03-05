#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>

#include "codegen.h"
#include "expr.h"
#include "global.h"
#include "token.h"
#include "varmap.h"

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
    if(reg == -1)
        return;

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

    for(size_t i = 0; i < varmap_len; ++i) {
        MapItem var = varmap[i];

        switch(var.type) {
            case VAL_INT:
                fprintf(out, "    g_%s: resd 1\n", var.identifier);
                break;
            case VAL_BOOL:
                fprintf(out, "    g_%s: resb 1\n", var.identifier);
                break;

            default:
                fprintf(stderr, "error: cannot generate code for variable of type %s\n", type_strs[var.type]);
                break;
        }
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

static void write_exit(FILE* out) {
    fprintf(out,
        "    mov rax, 60\n"
        "    mov rdi, 0\n"
        "    syscall\n"
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
        case VAL_IDENTIFIER:
            fprintf(out, "    mov %s, [g_%s]\n", registers[reg], expr->literal.value.identifier);
            break;
        case VAL_NONE:
        case VAL_ERROR:
            fprintf(stderr, "error: cannot generate code for value %s\n", type_strs[expr->literal.value.tag]);
            return -1;
    }

    return reg;
}

static int write_unary(Expr* expr, FILE* out) {
    const int rhs_reg = write_assembly_for_expr(expr->unary.rhs, out);
    switch(expr->unary.op.type) {
        case TOK_MINUS:
            fprintf(out, "    neg %s\n", registers[rhs_reg]);
            break;
        case TOK_NOT:
            fprintf(out,
                "    not %s\n"
                "    and %s, 1\n",
                registers[rhs_reg],
                registers[rhs_reg]
            );
            break;

        default:
            fprintf(stderr, "error: unknown unary operation '%s'\n", token_strs[expr->binary.op.type]);
            return -1;
    }
    return rhs_reg;
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
        case TOK_EQUAL_EQUAL:
            fprintf(out, 
                "    cmp %s, %s\n"
                "    mov %s, 0\n"
                "    mov rax, 1\n"
                "    cmove %s, rax\n",
                registers[lhs_reg],
                registers[rhs_reg],
                registers[lhs_reg],
                registers[lhs_reg]
            );
            break;
         case TOK_BANG_EQUAL:
            fprintf(out, 
                "    cmp %s, %s\n"
                "    mov %s, 0\n"
                "    mov rax, 1\n"
                "    cmovne %s, rax\n",
                registers[lhs_reg],
                registers[rhs_reg],
                registers[lhs_reg],
                registers[lhs_reg]
            );
            break;
        case TOK_LESS:
            fprintf(out, 
                "    cmp %s, %s\n"
                "    mov %s, 0\n"
                "    mov rax, 1\n"
                "    cmovl %s, rax\n",
                registers[lhs_reg],
                registers[rhs_reg],
                registers[lhs_reg],
                registers[lhs_reg]
            );
            break;
        case TOK_LESS_EQUAL:
            fprintf(out, 
                "    cmp %s, %s\n"
                "    mov %s, 0\n"
                "    mov rax, 1\n"
                "    cmovle %s, rax\n",
                registers[lhs_reg],
                registers[rhs_reg],
                registers[lhs_reg],
                registers[lhs_reg]
            );
            break;
        case TOK_GREATER:
            fprintf(out, 
                "    cmp %s, %s\n"
                "    mov %s, 0\n"
                "    mov rax, 1\n"
                "    cmovg %s, rax\n",
                registers[lhs_reg],
                registers[rhs_reg],
                registers[lhs_reg],
                registers[lhs_reg]
            );
            break;
        case TOK_GREATER_EQUAL:
            fprintf(out, 
                "    cmp %s, %s\n"
                "    mov %s, 0\n"
                "    mov rax, 1\n"
                "    cmovge %s, rax\n",
                registers[lhs_reg],
                registers[rhs_reg],
                registers[lhs_reg],
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

size_t if_counter = 0;

static int write_if(Expr* expr, FILE* out) {
    const size_t count = if_counter++;
    int cond_reg = write_assembly_for_expr(expr->if_stmt.condition, out);
    fprintf(out,
        "    cmp %s, 0\n"
        "    je _else_%lu\n",
        registers[cond_reg], count
    );
    for(size_t i = 0; i < expr->if_stmt.if_body_len; ++i)
        free_register(write_assembly_for_expr(expr->if_stmt.if_body[i], out));
    fprintf(out, "    jmp _end_%lu\n", count);

    fprintf(out, "_else_%lu:\n", count);
    for(size_t i = 0; i < expr->if_stmt.else_body_len; ++i)
        free_register(write_assembly_for_expr(expr->if_stmt.else_body[i], out));
    
    fprintf(out, "_end_%lu:\n", count);
    return -1;
}

static int write_variable_definition(Expr* expr, FILE* out) {
    if(!expr->var_def.initial_value)
        return -1;

    const int val_reg = write_assembly_for_expr(expr->var_def.initial_value, out);
    fprintf(out, "    mov [g_%s], %s\n", expr->var_def.identifier, registers[val_reg]);
    free_register(val_reg);

    return -1;
}

static int write_assign(Expr* expr, FILE* out) {
    const int val_reg = write_assembly_for_expr(expr->assign.expr, out);
    switch(expr->assign.op.type) {
        case TOK_EQUAL:
            fprintf(out, "    mov [g_%s], %s\n", expr->assign.identifier, registers[val_reg]);
            break;
        case TOK_PLUS_EQUAL: {
            const int temp = allocate_register();
            fprintf(out,
                "    mov %s, [g_%s]\n"
                "    add %s, %s\n"
                "    mov [g_%s], %s\n",
                registers[temp], expr->assign.identifier,
                registers[temp], registers[val_reg],
                expr->assign.identifier, registers[temp]
            );
            free_register(temp);
            break;
        }
        case TOK_MINUS_EQUAL: {
            const int temp = allocate_register();
            fprintf(out,
                "    mov %s, [g_%s]\n"
                "    sub %s, %s\n"
                "    mov [g_%s], %s\n",
                registers[temp], expr->assign.identifier,
                registers[temp], registers[val_reg],
                expr->assign.identifier, registers[temp]
            );
            free_register(temp);
            break;
        }
        case TOK_STAR_EQUAL: {
            const int temp = allocate_register();
            fprintf(out,
                "    mov %s, [g_%s]\n"
                "    imul %s, %s\n"
                "    mov [g_%s], %s\n",
                registers[temp], expr->assign.identifier,
                registers[temp], registers[val_reg],
                expr->assign.identifier, registers[temp]
            );
            free_register(temp);
            break;
        }
        case TOK_SLASH_EQUAL:
            fprintf(out,
                "    mov rax, [g_%s]\n"
                "    xor rdx, rdx\n"
                "    idiv %s\n"
                "    mov [g_%s], rax\n",
                expr->assign.identifier,
                registers[val_reg],
                expr->assign.identifier
            );
            break;
    }
    free_register(val_reg);
    return -1;
}

static size_t while_counter = 0;

static int write_while_loop(Expr* expr, FILE* out) {
    const size_t while_count = while_counter++;
    fprintf(out, "while_%lu:\n", while_count);

    const int cond_reg = write_assembly_for_expr(expr->while_loop.condition, out);
    fprintf(out,
        "    cmp %s, 0\n"
        "    jz while_%lu_end\n",
        registers[cond_reg],
        while_count
    );

    for(size_t i = 0; i < expr->while_loop.body_len; ++i)
        free_register(write_assembly_for_expr(expr->while_loop.body[i], out));

    fprintf(out,
        "    jmp while_%lu\n"
        "while_%lu_end:\n",
        while_count,
        while_count
    );
    
    free_register(cond_reg);
    return -1;
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
        case EXPR_IF:
            return write_if(expr, out);
        case EXPR_VAR_DEF:
            return write_variable_definition(expr, out);
        case EXPR_ASSIGN:
            return write_assign(expr, out);
        case EXPR_WHILE:
            return write_while_loop(expr, out);
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
        if(reg != -1)
            free_register(reg); // We won't be needing this register for now
    }

    write_exit(output_file);

    fclose(output_file);
    return true;
}
