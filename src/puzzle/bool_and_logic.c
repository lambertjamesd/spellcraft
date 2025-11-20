#include "bool_and_logic.h"

#include "../time/time.h"
#include "../cutscene/expression_evaluate.h"

void bool_and_logic_update(void* data) {
    struct bool_and_logic* logic = (struct bool_and_logic*)data;

    bool result = true;

    for (int i = 0; i < MAX_BOOL_INPUT; i += 1) {
        boolean_variable var = logic->input[i];

        if (var == VARIABLE_DISCONNECTED) {
            continue;
        }

        bool value = expression_get_bool(var);

        if (logic->input_invert[i]) {
            value = !value;
        }

        if (!value) {
            result = false;
            break;
        }
    }

    if (result) {
        expression_set_bool(logic->output, true);
    } else if (logic->should_unset) {
        expression_set_bool(logic->output, false);
    }
}

void bool_and_logic_init(struct bool_and_logic* logic, struct bool_and_logic_definition* definition, entity_id id) {
    memcpy(logic, definition, sizeof(struct bool_and_logic_definition));
    update_add(logic, bool_and_logic_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);
}

void bool_and_logic_destroy(struct bool_and_logic* logic) {
    update_remove(logic);
}

void bool_and_logic_common_init() {

}

void bool_and_logic_common_destroy() {

}