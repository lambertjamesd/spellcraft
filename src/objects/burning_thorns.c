#include "burning_thorns.h"

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../collision/shapes/cylinder.h"
#include "../collision/collision_scene.h"
#include "../entity/entity_spawner.h"
#include "../resource/material_cache.h"
#include "../render/coloru8.h"

#define BURN_TIME           4.5f
#define TRANSITION_TIME     4.0f
#define NOT_BURNING         -1.0f

#define DARKEN_TIME         (BURN_TIME - TRANSITION_TIME)

static color_t prim_color = {255, 206, 133, 255};
static color_t color_black = {0, 0, 0, 0};

static color_t burn_away_start = {0, 0, 0, 128};
static color_t burn_away_end = {128, 128, 128, 0};

static uint8_t burning_thorn_offset(uint8_t input, uint8_t amount) {
    // if (amount > input) {
    //     return 0;
    // }

    return input + amount;
}

static struct dynamic_object_type burning_object_shape = {
    CYLINDER_COLLIDER(2.0f, 2.0f),
};

void burning_thorns_update(void* data) {
    burning_thorns_t* thorns = (burning_thorns_t*)data;

    if (thorns->burn_time == NOT_BURNING) {
        return;
    }

    thorns->burn_time -= fixed_time_step;

    if (thorns->burn_time <= 0.0f) {
        entity_despawn(thorns->health.entity_id);
    } else if (thorns->burn_time < TRANSITION_TIME) {
        thorns->renderable.force_material = thorns->burn_material;
        float lerp_value = thorns->burn_time * (1.0f / TRANSITION_TIME);

        color_t burn_color = coloru8_lerp(&burn_away_end, &burn_away_start, lerp_value);
        burn_color.r = burning_thorn_offset(burn_color.r, 11);
        burn_color.g = burning_thorn_offset(burn_color.g, 9);
        burn_color.b = burning_thorn_offset(burn_color.b, 7);
        thorns->attrs[0].color = burn_color;
    } else {
        float lerp_value = (thorns->burn_time - TRANSITION_TIME) * (1.0f / DARKEN_TIME);
        thorns->attrs[0].color = coloru8_lerp(&color_black, &prim_color, lerp_value);
    }

}

float burning_thorns_damage(void* data, struct damage_info* damage) {
    burning_thorns_t* thorns = (burning_thorns_t*)data;
    if (damage->type & DAMAGE_TYPE_FIRE && thorns->burn_time == NOT_BURNING) {
        // struct Vector3 burn_pos;
        // vector3AddScaled(&thorns->transform.position, &gUp, 2.0f, &burn_pos);
        thorns->burn_time = BURN_TIME;
        // thorns->burning_effect = burning_effect_new(&burn_pos, thorns->transform.scale * 2.0f, BURN_TIME);
    }
    return 0.0f;
}

void burning_thorns_init(burning_thorns_t* thorns, struct burning_thorns_definition* definition, entity_id id) {
    transformSaInit(&thorns->transform, &definition->position, &definition->rotation, definition->scale);

    renderable_single_axis_init(&thorns->renderable, &thorns->transform, "rom:/meshes/objects/env_interactive/bramble_dry_barrier.tmesh");
    thorns->burn_material = material_cache_load("rom:/materials/temples/bramble_burnaway.mat");

    render_scene_add_renderable(&thorns->renderable, 1.4f);

    thorns->attrs[0].type = ELEMENT_ATTR_PRIM_COLOR;
    thorns->attrs[0].color = prim_color;
    thorns->attrs[1].type = ELEMENT_ATTR_NONE;

    thorns->renderable.attrs = thorns->attrs;
    thorns->burn_time = NOT_BURNING;
    thorns->burning_effect = NULL;

    update_add(thorns, burning_thorns_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);

    dynamic_object_init(
        id, 
        &thorns->collider, 
        &burning_object_shape, 
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY | COLLISION_LAYER_Z_TARGET, 
        &thorns->transform.position, 
        &thorns->transform.rotation
    );
    thorns->collider.scale = definition->scale;
    thorns->collider.is_fixed = 1;
    thorns->collider.weight_class = WEIGHT_CLASS_SUPER_HEAVY;
    thorns->collider.center.y = 2.0f;
    collision_scene_add(&thorns->collider);

    health_init(&thorns->health, id, 10.0f);
    health_set_callback(&thorns->health, burning_thorns_damage, thorns);
}

void burning_thorns_destroy(burning_thorns_t* thorns) {
    render_scene_remove(&thorns->renderable);
    renderable_destroy(&thorns->renderable);
    update_remove(thorns);
    collision_scene_remove(&thorns->collider);
    health_destroy(&thorns->health);
    material_cache_release(thorns->burn_material);

    if (thorns->burning_effect) {
        burning_effect_free(thorns->burning_effect);
    }
}