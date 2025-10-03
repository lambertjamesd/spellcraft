#include "burning_thorns.h"

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../collision/shapes/box.h"
#include "../collision/collision_scene.h"
#include "../entity/entity_spawner.h"

#define BURN_TIME   3.0f
#define NOT_BURNING -1.0f

static struct dynamic_object_type burning_object_shape = {
    BOX_COLLIDER(1.0f, 1.0f, 0.1f),
};

void burning_thorns_update(void* data) {
    burning_thorns_t* thorns = (burning_thorns_t*)data;

    if (thorns->burn_time == NOT_BURNING) {
        return;
    }

    thorns->burn_time -= fixed_time_step;

    uint8_t color_value = (uint8_t)((255.0f / BURN_TIME) * thorns->burn_time);
    thorns->attrs[0].color.r = color_value;
    thorns->attrs[0].color.g = color_value;
    thorns->attrs[0].color.b = color_value;

    if (thorns->burn_time <= 0.0f) {
        entity_despawn(thorns->health.entity_id);
    }    
}

float burning_thorns_damage(void* data, struct damage_info* damage) {
    burning_thorns_t* thorns = (burning_thorns_t*)data;
    if (damage->type & DAMAGE_TYPE_FIRE && thorns->burn_time == NOT_BURNING) {
        thorns->burn_time = BURN_TIME;
        thorns->burning_effect = burning_effect_new(&thorns->transform.position, thorns->transform.scale, BURN_TIME);
    }
    return 0.0f;
}

void burning_thorns_init(burning_thorns_t* thorns, struct burning_thorns_definition* definition, entity_id id) {
    transformSaInit(&thorns->transform, &definition->position, &definition->rotation, definition->scale);

    renderable_single_axis_init(&thorns->renderable, &thorns->transform, "rom:/meshes/objects/burning_thorns.tmesh");

    render_scene_add_renderable(&thorns->renderable, 1.4f);

    thorns->attrs[0].type = ELEMENT_ATTR_PRIM_COLOR;
    thorns->attrs[0].color = (color_t){255, 255, 255, 255};
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

    if (thorns->burning_effect) {
        burning_effect_free(thorns->burning_effect);
    }
}