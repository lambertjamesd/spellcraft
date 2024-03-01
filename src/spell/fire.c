#include "fire.h"

#include "../render/render_scene.h"
#include "assets.h"
#include "../time/time.h"
#include "../math/mathf.h"

#define CYCLE_TIME  0.08f

#define FIRE_LENGTH         4.0f

#define MAX_RADIUS          0.5f
#define MAX_RANDOM_OFFSET   0.3f

#define START_FADE          0.75f

#define TIP_RISE            0.5f

void fire_render(struct fire* fire, struct render_batch* batch) {
    int particle_count = (int)(fire->total_time * (1.0f / CYCLE_TIME));
    int particle_offset = 0;

    if (particle_count > MAX_FIRE_PARTICLE_COUNT) {
        particle_count = MAX_FIRE_PARTICLE_COUNT;
    }

    if (fire->end_time != -1.0f) {
        particle_offset = (fire->total_time - fire->end_time) * (1.0f / CYCLE_TIME);

        if (particle_offset > particle_count) {
            return;
        }

        particle_count -= particle_offset;
    }

    struct render_batch_billboard_element* element = render_batch_add_particles(batch, spell_assets_get()->fire_particle_mesh, particle_count);

    float time_lerp = fire->cycle_time * (1.0f / CYCLE_TIME);

    for (int i = 0; i < element->sprite_count; i += 1) {
        struct render_billboard_sprite* sprite = &element->sprites[i];

        float particle_time = (i + particle_offset + time_lerp) * (1.0f / MAX_FIRE_PARTICLE_COUNT);

        sprite->color.r = 255;
        sprite->color.g = 255;
        sprite->color.b = 255;
        sprite->color.a = 255;

        sprite->radius = particle_time * MAX_RADIUS;

        int final_index = i + fire->index_offset;

        if (final_index >= MAX_FIRE_PARTICLE_COUNT) {
            final_index -= MAX_FIRE_PARTICLE_COUNT;
        }

        vector3AddScaled(&fire->data_source->position, &fire->data_source->direction, particle_time * FIRE_LENGTH, &sprite->position);
        vector3AddScaled(&sprite->position, &fire->particle_offset[final_index], particle_time, &sprite->position);

        if (particle_time > START_FADE) {
            float alpha = 1.0f - (particle_time - START_FADE) * (1.0f / (1.0f - START_FADE));

            sprite->color.a = (uint8_t)(alpha * 255);
            sprite->position.y += TIP_RISE * (1.0f - alpha);
        }
    }
}

void fire_init(struct fire* fire, struct spell_data_source* source, struct spell_event_options event_options) {
    fire->render_id = render_scene_add(&r_scene_3d, &fire->data_source->position, 4.0f, (render_scene_callback)fire_render, fire);

    fire->data_source = source;
    spell_data_source_retain(source);

    fire->cycle_time = 0.0f;
    fire->total_time = 0.0f;
    fire->end_time = -1.0f;

    fire->index_offset = 0;

    for (int i = 0; i < MAX_FIRE_PARTICLE_COUNT; i += 1) {
        fire->particle_offset[i] = gZeroVec;

        struct Vector3* offset = &fire->particle_offset[fire->index_offset];
        offset->x = randomInRangef(-MAX_RANDOM_OFFSET, MAX_RANDOM_OFFSET);
        offset->y = randomInRangef(-MAX_RANDOM_OFFSET, MAX_RANDOM_OFFSET);
        offset->z = randomInRangef(-MAX_RANDOM_OFFSET, MAX_RANDOM_OFFSET);
    }
}

void fire_destroy(struct fire* fire) {
    render_scene_remove(&r_scene_3d, fire->render_id);
    spell_data_source_release(fire->data_source);
}

void fire_update(struct fire* fire, struct spell_event_listener* event_listener, struct spell_data_source_pool* pool) {
    fire->cycle_time += fixed_time_step;
    fire->total_time += fixed_time_step;

    if (fire->cycle_time > CYCLE_TIME) {
        fire->cycle_time -= CYCLE_TIME;

        if (fire->index_offset == 0) {
            fire->index_offset = MAX_FIRE_PARTICLE_COUNT - 1;
        } else {
            fire->index_offset -= 1;
        }
    }

    if (fire->end_time == -1 && fire->data_source->flags.cast_state != SPELL_CAST_STATE_ACTIVE) {
        fire->end_time = fire->total_time;
    }

    if (fire->end_time != -1 && fire->total_time > fire->end_time + CYCLE_TIME * MAX_FIRE_PARTICLE_COUNT) {
        spell_event_listener_add(event_listener, SPELL_EVENT_DESTROY, NULL);
    }
}