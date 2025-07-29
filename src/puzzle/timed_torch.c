#include "timed_torch.h"

#include <memory.h>
#include "../time/time.h"
#include "../cutscene/expression_evaluate.h"

void timed_torch_puzzle_update(void* data) {
    struct timed_torch_puzzle* puzzle = (struct timed_torch_puzzle*)data;

    if (expression_get_bool(puzzle->output)) {
        for (int i = 0; i < MAX_TIMED_TORCHES; i += 1) {
            boolean_variable var = puzzle->input[i];

            if (var == VARIABLE_DISCONNECTED) {
                continue;
            }

            if (!expression_get_bool(var)) {
                expression_set_bool(var, false);
                break;
            }
        }
    } else {
        bool should_light = true;

        for (int i = 0; i < MAX_TIMED_TORCHES; i += 1) {
            boolean_variable var = puzzle->input[i];

            if (var == VARIABLE_DISCONNECTED) {
                continue;
            }

            if (expression_get_bool(var)) {
                puzzle->input_timer[i] += fixed_time_step;

                if (puzzle->input_timer[i] > puzzle->torch_time) {
                    puzzle->input_timer[i] = 0.0f;
                    expression_set_bool(var, false);
                }
            } else {
                puzzle->input_timer[i] = 0.0f;
                should_light = false;
            }
        }

        if (should_light) {
            expression_set_bool(puzzle->output, true);
        }
    }
}

void timed_torch_puzzle_init(struct timed_torch_puzzle* torch_puzzle, struct timed_torch_puzzle_definition* definition) {
    torch_puzzle->position = definition->position;
    torch_puzzle->torch_time = definition->torch_time;
    torch_puzzle->output = definition->output;
    memcpy(&torch_puzzle->input, &definition->input_0, sizeof(boolean_variable) * MAX_TIMED_TORCHES);
    memset(&torch_puzzle->input_timer, 0, sizeof(float) * MAX_TIMED_TORCHES);

    update_add(torch_puzzle, timed_torch_puzzle_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);
}

void timed_torch_puzzle_destroy(struct timed_torch_puzzle* torch_puzzle) {
    update_remove(torch_puzzle);
}