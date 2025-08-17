#include "expression.h"

#include <libdragon.h>
#include <assert.h>
#include <malloc.h>


void expression_load(struct expression* expression, FILE* file) {
    int header;
    fread(&header, 1, 4, file);
    assert(header == EXPECTED_EXPR_HEADER);
    uint16_t byte_size;
    fread(&byte_size, 1, 2, file);
    expression->expression_program = malloc(byte_size);
    fread(expression->expression_program, 1, byte_size, file);
}

void expression_destroy(struct expression* expression) {
    free(expression->expression_program);
    expression->expression_program = NULL;
}

void expression_load_literal(struct expression* expression, int literal) {
    expression->expression_program = malloc(6);
    char* program = expression->expression_program;
    ((char*)program)[0] = EXPRESSION_TYPE_LOAD_LITERAL;
    memcpy(&program[1], &literal, 4);
    program[5] = EXPRESSION_TYPE_END;
}