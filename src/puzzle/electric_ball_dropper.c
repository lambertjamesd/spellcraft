#include "electric_ball_dropper.h"

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../cutscene/expression_evaluate.h"
#include "../collision/collision_scene.h"
#include "../entity/entity_spawner.h"

void electric_ball_dropper_check_for_drop(electric_ball_dropper_t* dropper) {
    bool is_active = expression_get_bool(dropper->is_active);
    bool rising_edge = is_active && !dropper->last_is_active;
    dropper->last_is_active = is_active;

    if (dropper->is_active != VARIABLE_DISCONNECTED && !is_active) {
        return;
    }

    if (rising_edge && dropper->current_ball) {
        entity_despawn(dropper->current_ball);
        dropper->current_ball = 0;
    }

    if (collision_scene_find_object(dropper->current_ball)) {
        return;
    }

    struct electric_ball_dropper_definition ball_definition = {
        .position = dropper->transform.position,
        .is_active = false,
    };

    dropper->current_ball = entity_spawn(ENTITY_TYPE_electric_ball, &ball_definition);
}

void electric_ball_dropper_update(void* data) {
    electric_ball_dropper_t* dropper = (electric_ball_dropper_t*)data;
    electric_ball_dropper_check_for_drop(dropper);
}

void electric_ball_dropper_init(electric_ball_dropper_t* dropper, struct electric_ball_dropper_definition* definition, entity_id entity_id) {
    transformSaInit(&dropper->transform, &definition->position, &gRight2, 1.0f);
    renderable_single_axis_init(&dropper->renderable, &dropper->transform, "rom:/meshes/puzzle/electric_ball_dropper.tmesh");
    render_scene_add_renderable(&dropper->renderable, 1.0f);
    update_add(dropper, electric_ball_dropper_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);
    dropper->current_ball = 0;
    dropper->is_active = definition->is_active;
    dropper->last_is_active = false;
    electric_ball_dropper_check_for_drop(dropper);
}

void electric_ball_dropper_destroy(electric_ball_dropper_t* dropper) {
    renderable_destroy(&dropper->renderable);
    render_scene_remove(&dropper->renderable);
    update_remove(dropper);
    entity_despawn(dropper->current_ball);
}