#include "defs.h"

void pack_position_vector(struct Vector3* input, short output[3]) {
    output[0] = (short)(input->x * SCENE_SCALE);
    output[1] = (short)(input->y * SCENE_SCALE);
    output[2] = (short)(input->z * SCENE_SCALE);
}