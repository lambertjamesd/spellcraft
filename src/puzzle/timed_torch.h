#ifndef __PUZZLE_TIMED_TORCH_H__
#define __PUZZLE_TIMED_TORCH_H__

#include "../scene/scene_definition.h"

#define MAX_TIMED_TORCHES   7

struct timed_torch_puzzle {
    struct Vector3 position;
    float torch_time;
    boolean_variable output;
    boolean_variable input[MAX_TIMED_TORCHES];
    float input_timer[MAX_TIMED_TORCHES];
};

void timed_torch_puzzle_init(struct timed_torch_puzzle* torch_puzzle, struct timed_torch_puzzle_definition* definition);
void timed_torch_puzzle_destroy(struct timed_torch_puzzle* torch_puzzle);

#endif