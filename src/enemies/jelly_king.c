#include "jelly_king.h"

#include "../collision/shapes/cylinder.h"
#include "../render/render_scene.h"
#include "../resource/animation_cache.h"
#include "../time/time.h"
#include "../collision/collision_scene.h"

static struct dynamic_object_type jelly_king_collider = {
    CYLINDER_COLLIDER(2.0f, 1.5f),
    .friction = 0.5f,
    .bounce = 0.0f,
    // about a 40 degree slope
    .max_stable_slope = 0.219131191f,
};

void jelly_king_update(void* data) {
    struct jelly_king* jelly_king = (struct jelly_king*)data;
    animator_update(&jelly_king->animator, jelly_king->renderable.armature.pose, fixed_time_step);
}

void jelly_king_init(struct jelly_king* jelly_king, struct jelly_king_definition* definition) {
    jelly_king->transform.position = definition->position;
    jelly_king->transform.rotation = definition->rotation;

    renderable_single_axis_init(&jelly_king->renderable, &jelly_king->transform, "rom:/meshes/enemies/jelly_king.tmesh");

    render_scene_add_renderable(&jelly_king->renderable, 3.0f);
    
    jelly_king->animation_set = animation_cache_load("rom:/meshes/enemies/jelly_king.anim");
    jelly_king->animations.idle = animation_set_find_clip(jelly_king->animation_set, "idle");

    animator_init(&jelly_king->animator, jelly_king->renderable.armature.bone_count);
    animator_run_clip(&jelly_king->animator, jelly_king->animations.idle, 0.0f, true);

    update_add(jelly_king, jelly_king_update, UPDATE_PRIORITY_SPELLS, UPDATE_LAYER_WORLD);

    dynamic_object_init(
        entity_id_new(),
        &jelly_king->collider,
        &jelly_king_collider,
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY,
        &jelly_king->transform.position,
        &jelly_king->transform.rotation
    );
    jelly_king->collider.center.y = jelly_king_collider.data.cylinder.half_height;

    collision_scene_add(&jelly_king->collider);
}

void jelly_king_destroy(struct jelly_king* jelly_king) {
    render_scene_remove(&jelly_king->renderable);
    renderable_destroy(&jelly_king->renderable);
    update_remove(jelly_king);
    collision_scene_remove(&jelly_king->collider);
}