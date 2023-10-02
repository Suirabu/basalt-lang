#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "expr.h"
#include "global.h"
#include "lexer.h"
#include "parser.h"
#include "sema.h"
#include "symbol.h"
#include "token.h"
#include "typecheck.h"

const char* stem(const char* filepath) {
    size_t start = strlen(filepath);
    while(start > 0) {
        if(filepath[start] == '/') {
            ++start;
            break;
        }
        --start;
    }

    size_t end = start;
    while(filepath[end] != '\0') {
        if(filepath[end] == '.')
            break;
        ++end;
    }

    const size_t len = end - start;
    char* result = malloc(len + 1);
    memcpy(result, filepath + start, len);
    result[len] = '\0';

    return result;
}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        fprintf(stderr, "error: no input file provided\n");
        return EXIT_FAILURE;
    }

    // Read source from disk
    const char* source_path = argv[1];
    FILE* source_file = fopen(source_path, "r");
    if(!source_file) {
        fprintf(stderr, "error: failed to open file '%s' for reading\n", source_path);
        return EXIT_FAILURE;
    }

    fseek(source_file, 0, SEEK_END);
    const size_t source_len = ftell(source_file);
    rewind(source_file);

    char* source = malloc(source_len + 1);
    assert(fread(source, source_len, 1, source_file) == 1);
    source[source_len] = '\0';
    fclose(source_file);

    // Lex source
    Lexer lexer = (Lexer) {
        .source = source,
        .source_path = source_path,
    };

    Token* tokens = NULL;
    size_t n_tokens = 0;
    Token current_token;
    bool has_error = false;

    do {
        if(!lexer_collect_token(&lexer, &current_token)) {
            has_error = true;
            continue;
        }

        tokens = realloc(tokens, sizeof(Token) * (n_tokens + 1));
        tokens[n_tokens++] = current_token;
    } while(current_token.type != TOK_EOF);
    
    free(source);

    if(has_error) {
        free(tokens);
        return EXIT_FAILURE;
    }

    Parser parser = (Parser) { .tokens = tokens };

    Expr** exprs = NULL;
    size_t n_exprs = 0;
    Expr* current_expr;

    while(!parser_reached_end(&parser)) {
        current_expr = parser_collect_expr(&parser);
        if(!current_expr) {
            has_error = true;
            continue;
        }

        exprs = realloc(exprs, sizeof(Expr*) * (n_exprs + 1));
        exprs[n_exprs++] = current_expr;
    }

    // Ensure `main` function exists and has the correct signature
    if(!symbol_exists("main")) {
        fprintf(stderr, "error: function `main` not defined\n");
        return 1;
    } else {
        const Symbol main_item = symbol_get("main");
        if(main_item.stype != SYM_FN || main_item.n_params != 0 || main_item.return_type != VAL_INT) {
            fprintf(stderr, "error: symbol `main` must be a function with no parameters and a return type of int\n");
            return 1;
        }
    }

    if(!typecheck_exprs(exprs, n_exprs)) {
        for(size_t i = 0; i < n_exprs; ++i)
            expr_free(exprs[i]);

        free(exprs);
        global_free_all();
        return 1;
    }
    
    if(!sema_analyze(exprs, n_exprs))
        return 1;

    char path_buffer[128];
    const char* source_path_stem = stem(source_path);
    snprintf(path_buffer, 127, "%s.asm", source_path_stem);

    generate_assembly(exprs, n_exprs, path_buffer);
    free(tokens);

    // Use buffer for commands
    char command_buffer[256];

    snprintf(command_buffer, 255, "yasm -f elf64 %s.asm -o %s.o", source_path_stem, source_path_stem);
    printf("cmd: %s\n", command_buffer);
    assert(system(command_buffer) == 0);
    snprintf(command_buffer, 255, "ld %s.o -o %s", source_path_stem, source_path_stem);
    printf("cmd: %s\n", command_buffer);
    assert(system(command_buffer) == 0);

    snprintf(path_buffer, 127, "%s.o", source_path_stem);
    remove(path_buffer);

    free((void*)source_path_stem);

    for(size_t i = 0; i < n_exprs; ++i) {
        expr_free(exprs[i]);
    }
    free(exprs);

    global_free_all();

    return EXIT_SUCCESS;
}
