#ifndef __OBJECT_MANA_GEM_H__
#define __OBJECT_MANA_GEM_H__

#include "../math/transform_single_axis.h"
#include "../render/renderable.h"
#include "../collision/dynamic_object.h"
#include "../collision/spatial_trigger.h"

struct mana_gem {
    struct TransformSingleAxis transform;
    struct spatial_trigger trigger;
    struct Vector3 velocity;
    float mana_amount;
    float radius;
};

struct mana_gem* mana_gem_new(struct Vector3* position, float mana_amount);
void mana_gem_free(struct mana_gem* gem);

void mana_gem_init(struct mana_gem* gem, struct Vector3* position, float mana_amount);
void mana_gem_destroy(struct mana_gem* gem);

#endif