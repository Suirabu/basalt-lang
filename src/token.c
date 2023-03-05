#include <stdio.h>

#include "token.h"

const char* token_strs[] = {
    "int",
    "bool",
    "string",
    "identifier",
    "left paren",
    "right paren",
    "equal",
    "equal equal",
    "bang equal",
    "less",
    "less equal",
    "greater",
    "greater equal",
    "plus",
    "plus equal",
    "minus",
    "minus equal",
    "star",
    "star equal",
    "slash",
    "slash equal",
    "colon",
    "if",
    "else",
    "then",
    "end",
    "not",
    "var",
    "while",
    "do",
    "int",
    "bool",
    "error",
    "eof",
};

const char* type_strs[] = {
    "int",
    "bool",
    "string",
    "identifier",
    "none",
    "error",
};

ValueTag token_type_to_value_tag(TokenType type) {
    switch(type) {
        case TOK_TYPE_INT:
            return VAL_INT;
        case TOK_TYPE_BOOL:
            return VAL_BOOL;

        default:
            fprintf(stderr, "error: cannot convert token %s to tag\n", token_strs[type]);
            return VAL_ERROR;
    }
}
