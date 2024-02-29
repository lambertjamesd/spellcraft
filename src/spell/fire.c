#include "fire.h"

#include "../render/render_scene.h"
#include "assets.h"
#include "../time/time.h"

#define CYCLE_TIME  0.08f

#define MAX_PARTICLE_COUNT  8

#define FIRE_LENGTH         4.0f

#define MAX_RADIUS          0.5f

#define START_FADE          0.75f

#define TIP_RISE            0.5f

void fire_render(struct fire* fire, struct render_batch* batch) {
    int particle_count = (int)(fire->total_time * (1.0f / CYCLE_TIME));

    if (particle_count > MAX_PARTICLE_COUNT) {
        particle_count = MAX_PARTICLE_COUNT;
    }

    struct render_batch_billboard_element* element = render_batch_add_particles(batch, spell_assets_get()->fire_particle_mesh, particle_count);

    float time_lerp = fire->cycle_time * (1.0f / CYCLE_TIME);

    for (int i = 0; i < element->sprite_count; i += 1) {
        struct render_billboard_sprite* sprite = &element->sprites[i];

        float particle_time = (i + time_lerp) * (1.0f / MAX_PARTICLE_COUNT);

        sprite->color.r = 255;
        sprite->color.g = 255;
        sprite->color.b = 255;
        sprite->color.a = 255;

        sprite->radius = particle_time * MAX_RADIUS;

        vector3AddScaled(&fire->data_source->position, &fire->data_source->direction, particle_time * FIRE_LENGTH, &sprite->position);

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
    }
}