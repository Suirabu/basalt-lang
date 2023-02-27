#include <stdio.h>
#include <stdlib.h>

#include "codegen.h"
#include "expr.h"
#include "lexer.h"
#include "parser.h"
#include "token.h"
#include "typecheck.h"

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
    fread(source, source_len, 1, source_file);
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
    free(tokens);

    if(typecheck_exprs(exprs, n_exprs)) {
        generate_assembly(exprs, n_exprs, "output.asm");
    }

    for(size_t i = 0; i < n_exprs; ++i) {
        expr_free(exprs[i]);
    }
    free(exprs);

    return EXIT_SUCCESS;
}
