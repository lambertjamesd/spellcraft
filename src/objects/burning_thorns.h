#ifndef __OBJECT_BURNING_THORNS_H__
#define __OBJECT_BURNING_THORNS_H__

#include "../math/transform_single_axis.h"
#include "../render/renderable.h"
#include "../scene/scene_definition.h"
#include "../collision/dynamic_object.h"
#include "../entity/health.h"
#include "../effects/burning_effect.h"

struct burning_thorns {
    struct TransformSingleAxis transform;
    struct renderable renderable;
    material_t* burn_material;

    health_t health;
    dynamic_object_t collider;

    struct element_attr attrs[2];

    float burn_time;
    burning_effect_t *burning_effect;
};

typedef struct burning_thorns burning_thorns_t;

void burning_thorns_init(burning_thorns_t* thorns, struct burning_thorns_definition* definition, entity_id id);
void burning_thorns_destroy(burning_thorns_t* thorns);
void burning_thorns_common_init();
void burning_thorns_common_destroy();

#endif