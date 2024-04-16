#include "expression.h"

#include <libdragon.h>
#include <assert.h>
#include <malloc.h>

// EXPR
#define EXPECTED_HEADER 0x45585052

void expression_load(struct expression* expression, FILE* file) {
    int header;
    fread(&header, 1, 4, file);
    assert(header == EXPECTED_HEADER);
    uint16_t byte_size;
    fread(&byte_size, 1, 2, file);
    expression->expression_program = malloc(byte_size);
    fread(expression->expression_program, 1, byte_size, file);
}

void expression_destroy(struct expression* expression) {
    free(expression->expression_program);
    expression->expression_program = NULL;
}