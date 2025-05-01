#include "./living_sprite_definitions.h"
#include "../collision/shapes/capsule.h"

static struct living_sprite_definition sprite_definitions[] = {
    [ELEMENT_TYPE_FIRE] = {
        .element_type = ELEMENT_TYPE_FIRE,
        .collider_type = {
            .minkowsi_sum = capsule_minkowski_sum,
            .bounding_box = capsule_bounding_box,
            .data = {
                .capsule = {
                    .radius = 0.25f,
                    .inner_half_height = 0.1f,
                }
            },
            .friction = 0.5f,
            .bounce = 0.5f,
        },
        .damage = 10.0f,
        .model_file = "rom:/meshes/spell/fire_sprite.tmesh",
    },
    [ELEMENT_TYPE_ICE] = {
        .element_type = ELEMENT_TYPE_ICE,
        .collider_type = {
            .minkowsi_sum = capsule_minkowski_sum,
            .bounding_box = capsule_bounding_box,
            .data = {
                .capsule = {
                    .radius = 0.25f,
                    .inner_half_height = 0.1f,
                }
            },
            .friction = 0.5f,
            .bounce = 0.5f,
        },
        .damage = 10.0f,
        .model_file = "rom:/meshes/spell/ice_sprite.tmesh",
    },
    [ELEMENT_TYPE_LIGHTNING] = {
        .element_type = ELEMENT_TYPE_LIGHTNING,
        .collider_type = {
            .minkowsi_sum = capsule_minkowski_sum,
            .bounding_box = capsule_bounding_box,
            .data = {
                .capsule = {
                    .radius = 0.25f,
                    .inner_half_height = 0.1f,
                }
            },
            .friction = 0.5f,
            .bounce = 0.5f,
        },
        .damage = 10.0f,
        .model_file = "rom:/meshes/spell/lightning_sprite.tmesh",
    },
    [ELEMENT_TYPE_WATER] = {
        .element_type = ELEMENT_TYPE_WATER,
        .collider_type = {
            .minkowsi_sum = capsule_minkowski_sum,
            .bounding_box = capsule_bounding_box,
            .data = {
                .capsule = {
                    .radius = 0.25f,
                    .inner_half_height = 0.1f,
                }
            },
            .friction = 0.5f,
            .bounce = 0.5f,
        },
        .damage = 0.0f,
        .model_file = "rom:/meshes/spell/water_sprite.tmesh",
    },
    [ELEMENT_TYPE_AIR] = {
        .element_type = ELEMENT_TYPE_AIR,
        .collider_type = {
            .minkowsi_sum = capsule_minkowski_sum,
            .bounding_box = capsule_bounding_box,
            .data = {
                .capsule = {
                    .radius = 0.25f,
                    .inner_half_height = 0.1f,
                }
            },
            .friction = 0.5f,
            .bounce = 0.5f,
        },
        .damage = 0.0f,
        .model_file = "rom:/meshes/spell/wind_sprite.tmesh",
    },
};


struct living_sprite_definition* living_sprite_find_def(enum element_type element_type, bool has_air, bool has_fire, bool has_ice) {
    switch (element_type)
    {
    case ELEMENT_TYPE_FIRE:
        return has_ice ? &sprite_definitions[ELEMENT_TYPE_LIGHTNING] : &sprite_definitions[ELEMENT_TYPE_FIRE];
    case ELEMENT_TYPE_ICE:
        return has_fire ? &sprite_definitions[ELEMENT_TYPE_WATER] : &sprite_definitions[ELEMENT_TYPE_ICE];
    case ELEMENT_TYPE_LIGHTNING:
        return &sprite_definitions[ELEMENT_TYPE_LIGHTNING];
    case ELEMENT_TYPE_WATER:
        return &sprite_definitions[ELEMENT_TYPE_WATER];
    case ELEMENT_TYPE_AIR:
        return &sprite_definitions[ELEMENT_TYPE_AIR];
    default:
        return NULL;
    }
}