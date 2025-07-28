#ifndef __CUTSCENE_EXPRESSION_H__
#define __CUTSCENE_EXPRESSION_H__

#include <stdint.h>
#include <libdragon.h>

enum expression_type {
    EXPRESSION_TYPE_END,
    EXPRESSION_TYPE_LOAD_LOCAL,
    EXPRESSION_TYPE_LOAD_SCENE_VAR,
    EXPRESSION_TYPE_LOAD_GLOBAL,
    EXPRESSION_TYPE_LOAD_LITERAL,

    EXPRESSION_TYPE_AND,
    EXPRESSION_TYPE_OR,
    EXPRESSION_TYPE_NOT,

    EXPRESSION_TYPE_EQ,
    EXPRESSION_TYPE_NEQ,
    EXPRESSION_TYPE_GT,
    EXPRESSION_TYPE_GTE,

    EXPRESSION_TYPE_ADD,
    EXPRESSION_TYPE_SUB,
    EXPRESSION_TYPE_MUL,
    EXPRESSION_TYPE_DIV,
    EXPRESSION_TYPE_NEGATE,

    EXPRESSION_TYPE_GTF,
    EXPRESSION_TYPE_GTEF,

    EXPRESSION_TYPE_ADDF,
    EXPRESSION_TYPE_SUBF,
    EXPRESSION_TYPE_MULF,
    EXPRESSION_TYPE_DIVF,
    EXPRESSION_TYPE_NEGATEF,

    EXPRESSION_TYPE_ITOF,
    EXPRESSION_TYPE_FTOI,
};

union expression_data {
    struct {
        uint16_t data_type;
        uint16_t word_offset;
    } load_variable;
    int literal;
};

struct expression {
    void* expression_program;
};

// EXPR
#define EXPECTED_EXPR_HEADER 0x45585052

struct __attribute__((packed)) expression_header {
    uint32_t header;
    uint16_t len;
};

void expression_load(struct expression* expression, FILE* file);
void expression_destroy(struct expression* expression);

#endif