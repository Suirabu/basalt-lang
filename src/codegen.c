#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "expr.h"
#include "global.h"
#include "symbol.h"
#include "token.h"
#include "value.h"

static const char* qregs[] = { "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15" };          // 64-bit registers
static const char* dregs[] = { "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d" };  // 32-bit registers
static const char* wregs[] = { "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w" };  // 16-bit registers
static const char* bregs[] = { "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b" };  // 8-bit registers
#define n_regs (sizeof(qregs) / sizeof(char*))

static int free_regs[n_regs];
static size_t n_free_regs = n_regs;

static void initialize_registers(void) {
    for(size_t i = 0; i < n_free_regs; ++i) {
        free_regs[i] = i;
    }
}

static int allocate_register(void) {
    if(n_free_regs == 0) {
        fprintf(stderr, "error: failed to allocate register\n");
        return -1;
    }
    
    return free_regs[--n_free_regs];
}

static void free_register(int reg) {
    if(reg == -1)
        return;

    free_regs[n_free_regs++] = reg;
}

static const char* get_register(int reg, size_t size) {
    switch(size) {
        case 1: return bregs[reg];
        case 2: return wregs[reg];
        case 4: return dregs[reg];
        case 8: return qregs[reg];
        default:
            fprintf(stderr, "error: invalid register size %lu\n", size);
            exit(1);
    };
}

// TODO: It might make more sense for the caller of write functions to specify
// a return register rather than returning them as we do currently. This would
// both allow us to easily specify a return register for exprs with multiple
// returns and prevent us from having to return -1 from codegen functions for 
// exprs with no return values.

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

    for(size_t i = 0; i < symbol_table_len; ++i) {
        const Symbol item = symbol_table[i];

        if(item.stype == SYM_VAR) {
            switch(item.type) {
                case VAL_INT:
                    fprintf(out, "    g_%s: resq 1\n", item.identifier);
                    break;
                case VAL_BOOL:
                    fprintf(out, "    g_%s: resb 1\n", item.identifier);
                    break;

                default:
                    fprintf(stderr, "error: cannot generate code for variable of type %s\n", type_strs[item.type]);
                    break;
            }
        }

    }

    fprintf(out, "\n");
}

static void write_preamble(FILE* out) {
    fprintf(out,
        "section .text\n"
        "global _start\n\n"
        "_start:\n"
        "    call fn_main\n"
        // Exit syscall
        "    mov rdi, rax\n"
        "    mov rax, 60\n"
        "    syscall\n"
    );
}

static int write_literal(Expr* expr, FILE* out) {
    const int reg = allocate_register();
    switch(expr->literal.value.tag) {
        case VAL_INT:
            fprintf(out, "    mov %s, %i\n", get_register(reg, SIZE_INT), expr->literal.value.val_int);
            break;
        case VAL_BOOL:
            fprintf(out, "    mov %s, %i\n", get_register(reg, SIZE_BOOL), expr->literal.value.val_bool);
            break;
        case VAL_STRING:
            fprintf(out, "    mov %s, str_%lu\n", get_register(reg, SIZE_STRING), expr->literal.value.global_id);
            break;
        case VAL_IDENTIFIER:
            if(symbol_exists(expr->literal.value.identifier)) {
                const Symbol* symbol = symbol_get(expr->literal.value.identifier);
                if(symbol->stype != SYM_VAR) {
                    fprintf(stderr, "error: symbol '%s' is not a variable\n", expr->literal.value.identifier);
                    return -1;
                }
                fprintf(out, "    mov %s, [g_%s]\n", get_register(reg, get_type_size(symbol->type)), expr->literal.value.identifier);
            } else if(expr->parent_fn) {
                for(size_t i = 0; i < expr->parent_fn->n_params; ++i) {
                    if(strcmp(expr->parent_fn->param_identifiers[i], expr->literal.value.identifier) == 0) {
                        size_t parameter_offset = 0;
                        for(size_t j = 0; j < i; ++j) {
                            parameter_offset = get_type_size(expr->parent_fn->param_types[j]);
                        }
                        fprintf(out, "    mov %s, [rsp + %lu]\n", get_register(reg, get_type_size(expr->parent_fn->param_types[i])), parameter_offset);
                        break;
                    }
                }
            }
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
            fprintf(out, "    neg %s\n", get_register(rhs_reg, SIZE_INT));
            break;
        case TOK_NOT:
            fprintf(out,
                "    not %s\n"
                "    and %s, 1\n",
                get_register(rhs_reg, SIZE_BOOL),
                get_register(rhs_reg, SIZE_BOOL)
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
                get_register(lhs_reg, SIZE_INT),
                get_register(rhs_reg, SIZE_INT)
            );
            break;
        case TOK_MINUS:
            fprintf(out, "    sub %s, %s\n",
                get_register(lhs_reg, SIZE_INT),
                get_register(rhs_reg, SIZE_INT)
            );
            break;
        case TOK_STAR:
            fprintf(out, "    imul %s, %s\n",
                get_register(lhs_reg, SIZE_INT),
                get_register(rhs_reg, SIZE_INT)
            );
            break;
        case TOK_SLASH:
            fprintf(out, 
                "    mov rax, %s\n"
                "    xor rdx, rdx\n"
                "    idiv %s\n"
                "    mov %s, rax\n",
                get_register(lhs_reg, SIZE_INT),
                get_register(rhs_reg, SIZE_INT),
                get_register(lhs_reg, SIZE_INT)
            );
            break;
        case TOK_EQUAL_EQUAL:
            fprintf(out, 
                "    cmp %s, %s\n"
                "    mov %s, 0\n"
                "    mov rax, 1\n"
                "    cmove %s, rax\n",
                get_register(lhs_reg, SIZE_INT),
                get_register(rhs_reg, SIZE_INT),
                get_register(lhs_reg, SIZE_INT),
                get_register(lhs_reg, SIZE_INT)
            );
            break;
         case TOK_BANG_EQUAL:
            fprintf(out, 
                "    cmp %s, %s\n"
                "    mov %s, 0\n"
                "    mov rax, 1\n"
                "    cmovne %s, rax\n",
                get_register(lhs_reg, SIZE_INT),
                get_register(rhs_reg, SIZE_INT),
                get_register(lhs_reg, SIZE_INT),
                get_register(lhs_reg, SIZE_INT)
            );
            break;
        case TOK_LESS:
            fprintf(out, 
                "    cmp %s, %s\n"
                "    mov %s, 0\n"
                "    mov rax, 1\n"
                "    cmovl %s, rax\n",
                get_register(lhs_reg, SIZE_INT),
                get_register(rhs_reg, SIZE_INT),
                get_register(lhs_reg, SIZE_INT),
                get_register(lhs_reg, SIZE_INT)
            );
            break;
        case TOK_LESS_EQUAL:
            fprintf(out, 
                "    cmp %s, %s\n"
                "    mov %s, 0\n"
                "    mov rax, 1\n"
                "    cmovle %s, rax\n",
                get_register(lhs_reg, SIZE_INT),
                get_register(rhs_reg, SIZE_INT),
                get_register(lhs_reg, SIZE_INT),
                get_register(lhs_reg, SIZE_INT)
            );
            break;
        case TOK_GREATER:
            fprintf(out, 
                "    cmp %s, %s\n"
                "    mov %s, 0\n"
                "    mov rax, 1\n"
                "    cmovg %s, rax\n",
                get_register(lhs_reg, SIZE_INT),
                get_register(rhs_reg, SIZE_INT),
                get_register(lhs_reg, SIZE_INT),
                get_register(lhs_reg, SIZE_INT)
            );
            break;
        case TOK_GREATER_EQUAL:
            fprintf(out, 
                "    cmp %s, %s\n"
                "    mov %s, 0\n"
                "    mov rax, 1\n"
                "    cmovge %s, rax\n",
                get_register(lhs_reg, SIZE_INT),
                get_register(rhs_reg, SIZE_INT),
                get_register(lhs_reg, SIZE_INT),
                get_register(lhs_reg, SIZE_INT)
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
        get_register(cond_reg, SIZE_BOOL), count
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
    fprintf(out, "    mov [g_%s], %s\n", expr->var_def.identifier, get_register(val_reg, get_type_size(symbol_get(expr->var_def.identifier)->type)));
    free_register(val_reg);

    return -1;
}

static int write_assign(Expr* expr, FILE* out) {
    const int val_reg = write_assembly_for_expr(expr->assign.expr, out);
    const size_t var_size = get_type_size(symbol_get(expr->assign.identifier)->type);

    switch(expr->assign.op.type) {
        case TOK_EQUAL:
            fprintf(out, "    mov [g_%s], %s\n", expr->assign.identifier, get_register(val_reg, var_size));
            break;
        case TOK_PLUS_EQUAL: {
            const int temp = allocate_register();
            fprintf(out,
                "    mov %s, [g_%s]\n"
                "    add %s, %s\n"
                "    mov [g_%s], %s\n",
                get_register(temp, var_size), expr->assign.identifier,
                get_register(temp, var_size), get_register(val_reg, var_size),
                expr->assign.identifier, get_register(temp, var_size)
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
                get_register(temp, var_size), expr->assign.identifier,
                get_register(temp, var_size), get_register(val_reg, var_size),
                expr->assign.identifier, get_register(temp, var_size)
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
                get_register(temp, var_size), expr->assign.identifier,
                get_register(temp, var_size), get_register(val_reg, var_size),
                expr->assign.identifier, get_register(temp, var_size)
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
                get_register(val_reg, var_size),
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
        get_register(cond_reg, SIZE_BOOL),
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

static size_t get_fn_stack_size(Symbol* symbol) {
    size_t stack_size = 0;
    for(size_t i = 0; i < symbol->n_params; ++i) {
        stack_size += get_type_size(symbol->param_types[i]);
    }
    return stack_size;
}

// TODO: Parameters
static int write_fn_def(Expr* expr, FILE* out) {
    fprintf(out, "\nfn_%s:\n", expr->fn_def.identifier);

    const size_t stack_size = get_fn_stack_size(expr->parent_fn);
    fprintf(out, "    sub rsp, %lu\n", stack_size);
    
    for(size_t i = 0; i < expr->fn_def.body_len; ++i) {
        free_register(write_assembly_for_expr(expr->fn_def.body[i], out));
    }

    return -1;
}

static int write_return(Expr* expr, FILE* out) {
    const int reg = write_assembly_for_expr(expr->op_return.value_expr, out);
    fprintf(out,
        "    mov rax, %s\n"
        "    add rsp, %lu\n"
        "    ret\n",
        get_register(reg, SIZE_INT),
        get_fn_stack_size(expr->parent_fn)
    );
    free_register(reg);
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
        case EXPR_FN_DEF:
            return write_fn_def(expr, out);
        case EXPR_RETURN:
            return write_return(expr, out);
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

    fclose(output_file);
    return true;
}
