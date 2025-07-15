#include "../collision/collision_scene.h"
#include "../collision/shapes/capsule.h"
#include "../objects/mana_gem.h"
#include "./living_sprite_definitions.h"
#include "push_single_target.h"

static struct push_single_definition wind_push = {
    .acceleration = 10.0f,
    .top_speed = 16.0f,
    .time = 2.0f,
    .bursty = false,
};

static struct push_single_definition flaming_wind_push = {
    .acceleration = 0.0f,
    .top_speed = 10.0f,
    .time = 1.0f,
    .bursty = true,
};

static struct push_single_definition lightning_wind_push  = {
    .acceleration = 0.0f,
    .top_speed = 100.0f,
    .time = 0.1f,
    .bursty = true,
};

void sprite_element_damage(struct living_sprite* living_sprite, struct contact* contact, float portion) {
    struct damage_info damage = {
        .amount = living_sprite->definition->damage,
        .type = health_determine_damage_type(living_sprite->definition->element_type),
        .source = living_sprite->collider.entity_id,
        .direction = contact->normal,
    };

    health_damage_id(
        living_sprite->target, 
        &damage,
        NULL
    );
}

void sprite_wind_effect(struct living_sprite* living_sprite, struct contact* contact, float portion) {
    struct push_single_definition* definition = &wind_push;

    if (living_sprite->flags.has_fire) {
        definition = living_sprite->flags.has_ice ? &lightning_wind_push : &flaming_wind_push;
    } else {
        definition = &wind_push;
    }


    if (living_sprite->flags.is_mine) {
        struct dynamic_object* obj = collision_scene_find_object(contact->other_object);
        if (!obj) {
            return;
        }
        struct Vector3 wind_direction;
        vector3Sub(obj->position, &living_sprite->transform.position, &wind_direction);
        wind_direction.y = 0.0f;
        vector3Normalize(&wind_direction, &wind_direction);
        single_push_new(contact->other_object, &wind_direction, definition);
    } else {
        struct Vector3 wind_direction;
        vector3Negate(&contact->normal, &wind_direction);
        single_push_new(contact->other_object, &wind_direction, definition);
    }
}

void sprite_heal_effect(struct living_sprite* living_sprite, struct contact* contact, float portion) {
    struct health* health = health_get(contact->other_object);

    if (!health) {
        return;
    }

    health_heal(health, living_sprite->health.current_health * portion);
}

void sprite_life_steal_effect(struct living_sprite* living_sprite, struct contact* contact, float portion) {
    struct health* health = health_get(contact->other_object);

    if (!health) {
        return;
    }

    struct Vector3 location;

    struct dynamic_object* dynamic = collision_scene_find_object(contact->other_object);

    if (dynamic) {
        vector3Add(dynamic->position, &dynamic->center, &location);
    } else {
        vector3Add(&living_sprite->transform.position, &gUp, &location);
    }

    float damage_amount = health_damage(health, &(struct damage_info){
        .amount = living_sprite->health.current_health * portion,
        .direction = gZeroVec,
        .source = living_sprite->collider.entity_id,
        .type = DAMAGE_TYPE_STEAL,
    });

    mana_gem_new(&location, damage_amount);
}

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
        .on_contact = sprite_element_damage,
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
        .on_contact = sprite_element_damage,
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
        .on_contact = sprite_element_damage,
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
        .on_contact = sprite_element_damage,
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
        .on_contact = sprite_wind_effect,
    },
    [ELEMENT_TYPE_LIFE] = {
        .element_type = ELEMENT_TYPE_LIFE,
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
        .model_file = "rom:/meshes/spell/life_sprite.tmesh",
        .on_contact = sprite_heal_effect,
    },
};

static struct living_sprite_definition life_steal_sprite = {
    .element_type = ELEMENT_TYPE_LIFE,
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
    .model_file = "rom:/meshes/spell/life_steal_sprite.tmesh",
    .on_contact = sprite_life_steal_effect,
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
    case ELEMENT_TYPE_LIFE:
        return has_ice ? &life_steal_sprite : &sprite_definitions[ELEMENT_TYPE_LIFE];
    default:
        return NULL;
    }
}