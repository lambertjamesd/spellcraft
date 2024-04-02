#ifndef __SCENE_WORLD_DEFINITION_H__
#define __SCENE_WORLD_DEFINITION_H__

#include "../math/vector3.h"
#include "../math/vector2.h"
#include <stdint.h>

struct crate_definition {
    struct Vector3 position;
    struct Vector2 rotation;
};

struct collectable_definition {
    struct Vector3 position;
};

struct biter_definition {
    struct Vector3 position;
    struct Vector2 rotation;
};

struct ground_torch_definition {
    struct Vector3 position;
    uint32_t is_lit;
};

#endif