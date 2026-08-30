#include "jelly_pot.h"    

#include "../collision/shapes/cylinder.h"
#include "../entity/entity_spawner.h"
#include "../enemies/jelly.h"

#define VISION_RADIUS   10.0f
#define ATTACK_INTERVAL 5.0f

#define LAUNCH_HEIGHT   2.0f

static tmesh_t* jelly_pot_mesh;

static dynamic_object_type_t jelly_pot_collider = {
    CYLINDER_COLLIDER(0.75f, 1.0f),
    .center = {0.0f, 1.0f, 0.0f},
    .friction = 0.9f,
    .bounce = 0.1f,
};

static spatial_trigger_type_t jelly_pot_vision = {
    .type = SPATIAL_TRIGGER_CYLINDER,
    .data = {
        .cylinder = {
            .radius = VISION_RADIUS,
            .half_height = VISION_RADIUS,
        },
    },
};

bool jelly_pot_fire(jelly_pot_t* jelly_pot) {
    int index = -1;

    for (int i = 0; i < MAX_ACTIVE_JELLIES; i += 1) {
        if (!entity_get(jelly_pot->spawned[i])) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        return false;
    }

    vector3_t offset;
    dynamic_object_t* obj = vision_update_current_target(&jelly_pot->target, &jelly_pot->vision, VISION_RADIUS, &offset);

    if (!obj) {
        return false;
    }

    vector3_t fire_from = jelly_pot->transform.position;
    fire_from.y += LAUNCH_HEIGHT;

    jelly_pot->spawned[index] = jelly_launch_at(
        &fire_from,
        &offset,
        obj->entity_id,
        jelly_pot->collider.collision_group
    );

    return jelly_pot->spawned[index] != 0;
}

void jelly_pot_update(void* data) {
    jelly_pot_t* jelly_pot = (jelly_pot_t*)data;

    if (!health_is_alive(&jelly_pot->health)) {
        entity_despawn(jelly_pot->collider.entity_id);
    }

    if (jelly_pot->attack_timer <= 0.0f) {
        if (jelly_pot_fire(jelly_pot)) {
            jelly_pot->attack_timer = ATTACK_INTERVAL;
        }
    } else {
        jelly_pot->attack_timer -= fixed_time_step;
    }
}
    
void jelly_pot_init(jelly_pot_t* jelly_pot, struct jelly_pot_definition* definition, entity_id entity_id) {
    transformSaInit(&jelly_pot->transform, &definition->position, &definition->rotation, 1.0f);

    render_scene_init_add_renderable(&jelly_pot->renderable, &jelly_pot->transform, jelly_pot_mesh, 2.0f);

    dynamic_object_init(entity_id, &jelly_pot->collider, &jelly_pot_collider, COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY, &jelly_pot->transform.position, NULL);
    jelly_pot->collider.weight_class = WEIGHT_CLASS_HEAVY;
    jelly_pot->collider.collision_group = entity_id;

    collision_scene_add(&jelly_pot->collider);

    spatial_trigger_init(
        &jelly_pot->vision,
        &jelly_pot->transform,
        &jelly_pot_vision,
        COLLISION_LAYER_DAMAGE_PLAYER,
        entity_id
    );

    collision_scene_add_trigger(&jelly_pot->vision);

    health_init(&jelly_pot->health, entity_id, 30.0f);

    update_add(jelly_pot, jelly_pot_update, UPDATE_PRIORITY_ENEMY, UPDATE_LAYER_WORLD);

    for (int i = 0; i < MAX_ACTIVE_JELLIES; i += 1) {
        jelly_pot->spawned[i] = 0;
    }

    jelly_pot->attack_timer = 0.0f;
    jelly_pot->target = 0;
}

void jelly_pot_destroy(jelly_pot_t* jelly_pot, struct jelly_pot_definition* definition) {
    render_scene_remove_renderable(&jelly_pot->renderable);
    collision_scene_remove(&jelly_pot->collider);
    health_destroy(&jelly_pot->health);
    update_remove(jelly_pot);
}

void jelly_pot_common_init() {
    jelly_pot_mesh = tmesh_cache_load("rom:/meshes/enemies/jelly_pot.tmesh");
}

void jelly_pot_common_destroy() {
    tmesh_cache_release(jelly_pot_mesh);
}
