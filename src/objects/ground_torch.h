#ifndef __OBJECTS_GROUND_TORCH_H__
#define __OBJECTS_GROUND_TORCH_H__

#include "../scene/scene_definition.h"
#include "../math/transform_single_axis.h"
#include "../render/renderable.h"
#include "../collision/dynamic_object.h"
#include "../entity/health.h"

struct ground_torch {
    struct Vector3 position;
    struct tmesh* base_mesh;
    struct tmesh* flame_mesh;
    
    struct dynamic_object dynamic_object;
    struct health health;

    uint16_t is_lit: 1;
};

void ground_torch_init(struct ground_torch* ground_torch, struct ground_torch_definition* definition);
void ground_torch_destroy(struct ground_torch* ground_torch);

#endif