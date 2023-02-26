#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "token.h"

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

    Token token;
    do {
        if(!lexer_collect_token(&lexer, &token))
            continue;

        printf("%s:%lu:%lu: %s",
            token.source_path,
            token.line + 1,
            token.column + 1,
            token_strs[token.type]
        );

        if(token.type == TOK_INT) {
            printf(" = %i\n", token.value);
        } else {
            printf("\n");
        }
    } while(token.type != TOK_EOF);

    free(source);
    return EXIT_SUCCESS;
}
