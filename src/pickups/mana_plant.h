#ifndef __PICKUP_MANA_PLANT_H__
#define __PICKUP_MANA_PLANT_H__

#include "../render/renderable.h"
#include "../scene/scene_definition.h"
#include "../math/transform_single_axis.h"
#include "../collision/spatial_trigger.h"

struct mana_plant {
    struct TransformSingleAxis transform; 
    struct tmesh* mesh;
    struct spatial_trigger trigger;
    float respawn_timer;
};

void mana_plant_init(struct mana_plant* plant, struct mana_plant_definition* definition);
void mana_plant_destroy(struct mana_plant* plant);

#endif