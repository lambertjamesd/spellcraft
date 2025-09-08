#include "lightning_storm.h"

#include "../collision/collision_scene.h"
#include "../render/render_batch.h"
#include "../render/render_scene.h"

static spatial_trigger_type_t lightning_storm_shape = {SPATIAL_TRIGGER_CYLINDER(2.0f, 2.0f)};

void lightning_storm_render(void* data, render_batch_t* batch) {

}

void lightning_storm_init(struct lightning_storm* storm, struct spell_data_source* source, struct spell_event_options event_options) {
    transformSaInit(&storm->transform, &source->position, &gRight2, 1.0f);
    spatial_trigger_init(&storm->trigger, &storm->transform, &lightning_storm_shape, COLLISION_LAYER_DAMAGE_ENEMY);
    collision_scene_add_trigger(&storm->trigger);

    storm->data_source = spell_data_source_retain(source);

    spell_data_source_request_animation(source, SPELL_ANIMATION_SWING);
    render_scene_add(&storm->transform.position, 2.0f, lightning_storm_render, storm);

    storm->next_strike = 0;
    storm->last_strike = 0;
    storm->remaining_strike_count = 0;
    storm->next_target_index = 0;
    storm->total_target_count = 0;
}

void lightning_storm_destroy(struct lightning_storm* storm) {
    collision_scene_remove_trigger(&storm->trigger);
    render_scene_remove(storm);
    spell_data_source_release(storm->data_source);
}

bool lightning_storm_update(struct lightning_storm* storm) {
    if (storm->remaining_strike_count == 0 && storm->data_source->flags.cast_state == SPELL_CAST_STATE_ACTIVE) {
        
    }

    return false;
}