#ifndef __OBJECTS_GROUND_TORCH_H__
#define __OBJECTS_GROUND_TORCH_H__

#include "../scene/scene_definition.h"
#include "../math/transform_single_axis.h"
#include "../render/renderable.h"
#include "../collision/dynamic_object.h"
#include "../entity/health.h"

struct ground_torch {
    transform_sa_t transform;
    struct tmesh* base_mesh;
    struct tmesh* flame_mesh;
    
    struct dynamic_object dynamic_object;
    struct health health;

    enum ground_torch_type torch_type;

    boolean_variable lit_source;
};

void ground_torch_init(struct ground_torch* ground_torch, struct ground_torch_definition* definition, entity_id id);
void ground_torch_destroy(struct ground_torch* ground_torch);
void ground_torch_common_init();
void ground_torch_common_destroy();

#endif