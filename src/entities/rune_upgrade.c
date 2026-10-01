#include "rune_upgrade.h"    
    
#include "../collision/shapes/cylinder.h"

static dynamic_object_type_t rune_upgrade_collider = {
    CYLINDER_COLLIDER(0.9f, 1.0f),
    .center = {0.0f, 1.0f, 0.0f},  
};

static vector3_t player_cutscene_offset = {
    0.0f,
    0.0f,
    1.8f,
};

#define FADE_OUT_START_TIME 2.2f
#define FADE_OUT_END_TIME   2.9f

void rune_upgrade_update(void* data) {
    rune_upgrade_t* rune_upgrade = (rune_upgrade_t*)data;

    if (rune_upgrade->interactable.interact_type == INTERACT_TYPE_NONE && !animator_is_running(&rune_upgrade->animator)) {
        rune_upgrade->attrs[0].color = (color_t){0, 0, 0, 255};
        return;
    }

    animator_update(&rune_upgrade->animator, fixed_time_step);

    if (rune_upgrade->animator.current_time < FADE_OUT_START_TIME) {
        rune_upgrade->attrs[0].color = (color_t){0, 0, 0, (sinf(scene_time) + 1.0f) * 24.f};
    } else if (rune_upgrade->animator.current_time < FADE_OUT_END_TIME) {
        rune_upgrade->attrs[0].color = (color_t){0, 0, 0, (uint8_t)((rune_upgrade->animator.current_time - FADE_OUT_START_TIME) * (255.0f / (FADE_OUT_END_TIME - FADE_OUT_START_TIME)))};
    } else {
        rune_upgrade->attrs[0].color = (color_t){0, 0, 0, 255};
    }
}

void rune_upgrade_revert_idle(void* data) {
    rune_upgrade_t* rune_upgrade = (rune_upgrade_t*)data;    
    animator_run_clip(&rune_upgrade->animator, animation_set_find_clip(rune_upgrade->animations, "idle"), 0.0f, false);
    rune_upgrade->animator.blend_frames = 0;
}

void rune_upgrade_interact(interactable_t* interactable, entity_id from) {
    rune_upgrade_t* rune_upgrade = (rune_upgrade_t*)interactable->data;

    if (expression_get_bool(rune_upgrade->has_item)) {
        return;
    }

    expression_set_integer(rune_upgrade->rune_level, expression_get_integer(rune_upgrade->rune_level) + 1);
    interactable->interact_type = INTERACT_TYPE_NONE;

    animator_run_clip(&rune_upgrade->animator, animation_set_find_clip(rune_upgrade->animations, "get_rune"), 0.0f, false);

    cutscene_builder_t cutscene;

    cutscene_builder_init(&cutscene);

    vector3_t player_pos;
    transformSaTransformPoint(&rune_upgrade->transform, &player_cutscene_offset, &player_pos);

    vector2_t player_rot;
    vector2Negate(&rune_upgrade->transform.rotation, &player_rot);

    cutscene_builder_pause(&cutscene, true, false);
    cutscene_builder_snap_to_pos(&cutscene, ENTITY_ID_PLAYER, &player_pos);
    cutscene_builder_snap_to_rot(&cutscene, ENTITY_ID_PLAYER, &player_rot);
    cutscene_builder_camera_animate(&cutscene, "get_rune", ENTITY_ID_PLAYER);
    cutscene_builder_npc_animate(&cutscene, ENTITY_ID_PLAYER, "get_rune", false);
    cutscene_builder_camera_wait(&cutscene);
    cutscene_builder_camera_return(&cutscene);
    cutscene_builder_callback(&cutscene, rune_upgrade_revert_idle, rune_upgrade);

    cutscene_builder_show_rune_upgrade(&cutscene, SPELL_SYMBOL_FIRE);

    cutscene_builder_delay(&cutscene, 3.0f);

    cutscene_builder_dialog(&cutscene, "You upgraded the fire rune");

    cutscene_runner_run(cutscene_builder_finish(&cutscene), 0, cutscene_runner_free_on_finish(), NULL, 0);
}

void rune_upgrade_init(rune_upgrade_t* rune_upgrade, struct rune_upgrade_definition* definition, entity_id entity_id) {
    transformSaInit(&rune_upgrade->transform, &definition->position, &definition->rotation, 1.0f);
    rune_upgrade->has_item = definition->has_item;
    rune_upgrade->rune_level = definition->rune_level;

    renderable_single_axis_init(&rune_upgrade->renderable, &rune_upgrade->transform, definition->mesh);
    render_scene_add_renderable(&rune_upgrade->renderable, 0.0f);

    bool has_item = expression_get_bool(definition->has_item);

    rune_upgrade->attrs[0] = (element_attr_t){.type = ELEMENT_ATTR_ENV_COLOR, .color = {0, 0, 0, has_item ? 255 : 0}};
    rune_upgrade->attrs[1] = (element_attr_t){.type = ELEMENT_ATTR_NONE};
    rune_upgrade->renderable.attrs = &rune_upgrade->attrs[0];

    animator_init(&rune_upgrade->animator, rune_upgrade->renderable.mesh_render.armature.bone_count);
    renderable_set_animator(&rune_upgrade->renderable, &rune_upgrade->animator);
    update_add(rune_upgrade, rune_upgrade_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_CUTSCENE | UPDATE_LAYER_WORLD);

    rune_upgrade->animations = animation_cache_load("rom:/meshes/objects/rune_upgrade/fire.anim");

    dynamic_object_init(
        entity_id,
        &rune_upgrade->collider,
        &rune_upgrade_collider,
        COLLISION_LAYER_Z_TARGET | COLLISION_LAYER_TANGIBLE,
        &rune_upgrade->transform.position,
        &rune_upgrade->transform.rotation
    );

    rune_upgrade->collider.is_fixed = true;
    rune_upgrade->collider.weight_class = WEIGHT_CLASS_SUPER_HEAVY;

    collision_scene_add(&rune_upgrade->collider);

    interactable_init(
        &rune_upgrade->interactable, 
        entity_id, 
        has_item ? INTERACT_TYPE_NONE : INTERACT_TYPE_CHECK, 
        rune_upgrade_interact, 
        rune_upgrade
    );
}

void rune_upgrade_destroy(rune_upgrade_t* rune_upgrade, struct rune_upgrade_definition* definition) {
    renderable_destroy(&rune_upgrade->renderable);
    render_scene_remove(&rune_upgrade->renderable);

    collision_scene_remove(&rune_upgrade->collider);

    interactable_destroy(&rune_upgrade->interactable);

    animator_destroy(&rune_upgrade->animator);
    animation_cache_release(rune_upgrade->animations);
}

void rune_upgrade_common_init() {

}

void rune_upgrade_common_destroy() {

}
