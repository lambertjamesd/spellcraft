#include "fire_around.h"

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../collision/collision_scene.h"
#include "../entity/health.h"
#include "../render/defs.h"
#include "assets.h"

#include "fire.h"

#define ATTACK_RADIUS   2.0f
#define HALF_HEIGHT     0.5f

#define EXPAND_TIME     0.5f
#define DISAPPEAR_TIME  0.5f

static struct dynamic_object_type fire_around_object_type = {
    .minkowsi_sum = dynamic_object_cylinder_minkowski_sum,
    .bounding_box = dynamic_object_cylinder_bounding_box,
    .data = { 
        .cylinder = {
            .radius = ATTACK_RADIUS,
            .half_height = HALF_HEIGHT,
        },
    },
};

void fire_around_render(void* data, struct render_batch* batch) {
    struct fire_around* fire_around = (struct fire_around*)data;

    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    mat4x4 mtx;
    struct Vector3 scaledPosition;
    matrixFromScale(mtx, ATTACK_RADIUS);
    vector3Scale(&fire_around->position, &scaledPosition, SCENE_SCALE);
    matrixApplyPosition(mtx, &scaledPosition);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    render_batch_add_tmesh(batch, spell_assets_get()->fire_around_mesh, mtxfp, 1, NULL, NULL);
}

void fire_around_init(struct fire_around* fire_around, struct spell_data_source* source, struct spell_event_options event_options, enum element_type element_type) {
    fire_around->position = source->position;

    fire_around->data_source = source;
    spell_data_source_retain(source);
    fire_around->timer = 0.0f;

    render_scene_add(&fire_around->data_source->position, 4.0f, (render_scene_callback)fire_around_render, fire_around);

    dynamic_object_init(
        entity_id_new(), 
        &fire_around->dynamic_object, 
        &fire_around_object_type, 
        COLLISION_LAYER_DAMAGE_ENEMY,
        &fire_around->position, 
        NULL
    );
    fire_around->dynamic_object.is_trigger = 1;
    collision_scene_add(&fire_around->dynamic_object);

    fire_around->end_time = -1;
    fire_around->element_type = element_type;
}

void fire_around_destroy(struct fire_around* fire_around) {
    render_scene_remove(fire_around);
    spell_data_source_release(fire_around->data_source);
    collision_scene_remove(&fire_around->dynamic_object);
}

void fire_around_update(struct fire_around* fire_around, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    if (fire_around->end_time == -1 && fire_around->data_source->flags.cast_state != SPELL_CAST_STATE_ACTIVE) {
        fire_around->end_time = DISAPPEAR_TIME;
    }

    if (fire_around->end_time > 0.0f) {
        fire_around->end_time -= fixed_time_step;

        if (fire_around->end_time <= 0.0f) {
            spell_event_listener_add(event_listener, SPELL_EVENT_DESTROY, NULL, 0.0f);
        }
    }

    fire_around->position = fire_around->data_source->position;
    
    fire_apply_damage(&fire_around->dynamic_object, fire_determine_damage_type(fire_around->element_type));
}   