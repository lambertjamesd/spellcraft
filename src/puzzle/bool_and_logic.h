#ifndef __PUZZLE_BOOL_LOGIC_H__
#define __PUZZLE_BOOL_LOGIC_H__

#include "../scene/scene_definition.h"
#include "../entity/entity_id.h"

#define MAX_BOOL_INPUT  7

struct bool_and_logic {
    struct Vector3 position;
    boolean_variable output;
    boolean_variable input[MAX_BOOL_INPUT];
    bool should_unset;
    bool input_invert[MAX_BOOL_INPUT];
};

void bool_and_logic_init(struct bool_and_logic* logic, struct bool_and_logic_definition* definition, entity_id id);
void bool_and_logic_destroy(struct bool_and_logic* logic);
void bool_and_logic_common_init();
void bool_and_logic_common_destroy();

#endif