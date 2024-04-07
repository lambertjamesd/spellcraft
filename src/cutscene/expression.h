#ifndef __CUTSCENE_EXPRESSION_H__
#define __CUTSCENE_EXPRESSION_H__

#include <stdint.h>

enum expression_type {
    EXPRESSION_TYPE_END,
    EXPRESSION_TYPE_LOAD_LOCAL,
    EXPRESSION_TYPE_LOAD_GLOBAL,
    EXPRESSION_TYPE_LOAD_LITERAL,
    EXPRESSION_TYPE_AND,
    EXPRESSION_TYPE_OR,
    EXPRESSION_TYPE_NOT,
    EXPRESSION_TYPE_ADD,
};

union expression_data {
    struct {
        uint16_t data_type;
        uint16_t word_offset;
    } load_variable;
    int literal;
};

struct expression {
    uint8_t* steps;
    union expression_data* step_data;
};

#endif