#include "lightning_storm.h"

#include "../collision/collision_scene.h"
#include "../render/render_batch.h"
#include "../render/render_scene.h"
#include "../render/defs.h"
#include "../math/mathf.h"
#include "../time/time.h"
#include "assets.h"

static spatial_trigger_type_t lightning_storm_shape = {SPATIAL_TRIGGER_CYLINDER(2.0f, 2.0f)};
static spatial_trigger_type_t lightning_storm_damage_shape = {SPATIAL_TRIGGER_CYLINDER(0.25f, 0.25f)};

#define MIN_STRIKE_INTERVAL 0.125f
#define MAX_STRIKE_INTERVAL 0.25f

#define MTX_SCALE   4.0f

lightning_strike_t* lightning_advance_strike(lightning_storm_t* storm, lightning_strike_t* strike) {
    ++strike;
    if (strike == &storm->strikes[LIGHTNING_STORM_MAX_STRIKE_COUNT]) {
        return storm->strikes;
    }
    return strike;
}

int8_t lightning_pack_particle(float value) {
    float result = (1.0f / MTX_SCALE) * MODEL_SCALE * value;

    if (result > 127.0f) {
        return 127;
    }

    if (result < -128.0f) {
        return -128;
    }

    return (uint8_t)result;
}

void lightning_storm_render(void* data, render_batch_t* batch) {
    lightning_storm_t* storm = (lightning_storm_t*)data;
    lightning_strike_t* strike = &storm->strikes[storm->first_active_strike];
    for (int i = 0; i < storm->active_strike_count; i += 1) {
        lightning_strike_render(strike, batch);
        strike = lightning_advance_strike(storm, strike);
    }

    transform_sa_t transform = {
        .position = storm->data_source->position,
        .rotation = gRight2,
        .scale = MTX_SCALE,
    };

    T3DMat4FP* mtx = render_batch_transformfp_from_sa(batch, &transform);

    render_batch_element_t* element = render_batch_add_dynamic_particles(
        batch, 
        spell_assets_get()->thunder_cloud, 
        storm->active_strike_count, 
        &(struct render_batch_particle_size){
            .particle_size = 16,
            .particle_scale_height = 0xFFFF,
            .particle_scale_width = 0xFFFF,
        }, 
        mtx
    );

    
    strike = &storm->strikes[storm->first_active_strike];
    TPXParticle* particle = element->particles.particles->particles;
    for (int i = 0; i < storm->active_strike_count; i += 2) {
        struct Vector3 offset;
        vector3Sub(&strike->position, &storm->data_source->position, &offset);
        offset.y += 2.0f;

        particle->posA[0] = lightning_pack_particle(offset.x);
        particle->posA[1] = lightning_pack_particle(offset.y);
        particle->posA[2] = lightning_pack_particle(offset.z);
        particle->sizeA = (int8_t)(lightning_strike_cloud_size(strike) * 127.0f);
        particle->colorA[0] = 255;
        particle->colorA[1] = 255;
        particle->colorA[2] = 255;
        particle->colorA[3] = 0;

        strike = lightning_advance_strike(storm, strike);

        if (i + 1 < storm->active_strike_count) {
            vector3Sub(&strike->position, &storm->data_source->position, &offset);

            particle->posB[0] = lightning_pack_particle(offset.x);
            particle->posB[1] = lightning_pack_particle(offset.y);
            particle->posB[2] = lightning_pack_particle(offset.z);
            particle->sizeB = (int8_t)(lightning_strike_cloud_size(strike) * 127.0f);
            particle->colorB[0] = 255;
            particle->colorB[1] = 255;
            particle->colorB[2] = 255;
            particle->colorB[3] = 0;
            
            strike = lightning_advance_strike(storm, strike);
        } else {
            particle->posB[0] = 0;
            particle->posB[1] = 0;
            particle->posB[2] = 0;
            particle->sizeB = 0;
            particle->colorB[0] = 255;
            particle->colorB[1] = 255;
            particle->colorB[2] = 255;
            particle->colorB[3] = 0;
        }
    }
}

void lightning_storm_init(lightning_storm_t* storm, struct spell_data_source* source, struct spell_event_options event_options) {
    transformSaInit(&storm->transform, &source->position, &gRight2, 1.0f);
    spatial_trigger_init(&storm->trigger, &storm->transform, &lightning_storm_shape, COLLISION_LAYER_DAMAGE_ENEMY);
    collision_scene_add_trigger(&storm->trigger);

    storm->data_source = spell_data_source_retain(source);

    spell_data_source_request_animation(source, SPELL_ANIMATION_SWING);
    render_scene_add(&storm->transform.position, 2.0f, lightning_storm_render, storm);

    storm->first_active_strike = 0;
    storm->next_target_strike = 0;
    storm->active_strike_count = 0;
    storm->total_target_count = 0;
    storm->strike_timer = 0.0f;
}

void lightning_storm_destroy(lightning_storm_t* storm) {
    collision_scene_remove_trigger(&storm->trigger);
    render_scene_remove(storm);
    spell_data_source_release(storm->data_source);
}

void lightning_storm_setup_target(lightning_storm_t* storm) {
    storm->transform.position = storm->strikes[storm->next_target_strike].position;
}

void lightning_storm_start_strike(lightning_storm_t* storm) {
    if (storm->active_strike_count >= LIGHTNING_STORM_ACTIVE_STRIKE_COUNT || storm->total_target_count == 0) {
        return;
    }

    --storm->total_target_count;

    uint8_t next_index = storm->first_active_strike + storm->active_strike_count;

    if (next_index >= LIGHTNING_STORM_ACTIVE_STRIKE_COUNT) {
        next_index -= LIGHTNING_STORM_ACTIVE_STRIKE_COUNT;
    }

    lightning_strike_t* strike = &storm->strikes[next_index];

    dynamic_object_t* obj = collision_scene_find_object(storm->total_target_count);

    struct Vector3 position;

    if (!obj) {
        struct Vector2 randomPos;
        vector2RandomUnitCircle(&randomPos);

        position.x = storm->data_source->position.x + randomPos.x * lightning_storm_shape.data.cylinder.radius;
        position.y = storm->data_source->position.y + 1.0f;
        position.z = storm->data_source->position.z + randomPos.y * lightning_storm_shape.data.cylinder.radius;

        struct mesh_shadow_cast_result cast_result;
        if (collision_scene_shadow_cast(&position, &cast_result)) {
            position.y = cast_result.y;
        } else {
            position.y -= 1.0f;
        }
    } else {
        position = *obj->position;
    }

    lightning_strike_start(strike, &position);
    storm->trigger.type = &lightning_storm_damage_shape;
    lightning_storm_setup_target(storm);

    ++storm->active_strike_count;
    storm->strike_timer = randomInRangef(MIN_STRIKE_INTERVAL, MAX_STRIKE_INTERVAL);
}

void lightning_storm_find_targets(lightning_storm_t* storm, int strike_count) {
    contact_t* contact = storm->trigger.active_contacts;

    if (strike_count > LIGHTNING_STORM_MAX_STRIKE_COUNT) {
        strike_count = LIGHTNING_STORM_MAX_STRIKE_COUNT;
    }

    entity_id* next_target = storm->targets;

    while (strike_count) {
        if (contact) {
            *next_target = contact->other_object;
            contact = contact->next;
        } else {
            *next_target = 0;
            contact = storm->trigger.active_contacts;
        }

        ++next_target;
        --strike_count;
    }

    storm->total_target_count = next_target - storm->targets;

    lightning_storm_start_strike(storm);
}

bool lightning_storm_update(lightning_storm_t* storm) {
    if (storm->total_target_count == 0 && storm->active_strike_count == 0) {
        if (storm->data_source->flags.cast_state == SPELL_CAST_STATE_ACTIVE) {
            lightning_storm_find_targets(storm, 8);
        }
        return true;
    }

    storm->strike_timer -= fixed_time_step;

    if (storm->strike_timer <= 0.0f) {
        lightning_storm_start_strike(storm);
    }

    lightning_strike_t* strike = &storm->strikes[storm->first_active_strike];
    lightning_strike_t* next = &storm->strikes[storm->next_target_strike];
    for (int i = 0; i < storm->active_strike_count; i += 1) {
        if (lightning_strike_update(strike) && next == strike) {
            // apply damage

            ++storm->next_target_strike;
            if (storm->next_target_strike == LIGHTNING_STORM_ACTIVE_STRIKE_COUNT) {
                storm->next_target_strike = 0;
            }
            lightning_storm_setup_target(storm);
        }

        ++strike;
        if (strike == &storm->strikes[LIGHTNING_STORM_MAX_STRIKE_COUNT]) {
            strike = storm->strikes;
        }
    }

    if (!lightning_strike_is_active(&storm->strikes[storm->first_active_strike])) {
        ++storm->first_active_strike;
        --storm->active_strike_count;
        if (storm->first_active_strike == LIGHTNING_STORM_ACTIVE_STRIKE_COUNT) {
            storm->first_active_strike = 0;
        }
    }

    return storm->active_strike_count > 0;
}