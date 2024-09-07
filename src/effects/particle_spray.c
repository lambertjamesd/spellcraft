#include "particle_spray.h"

#include "../time/time.h"
#include "../render/render_scene.h"
#include "../render/render_batch.h"
#include "../math/mathf.h"
#include "effect_allocator.h"

#define CYCLE_TIME  0.08f

#define FIRE_LENGTH         4.0f

#define MAX_RADIUS          0.5f
#define MAX_RANDOM_OFFSET   0.3f

#define START_FADE          0.75f

#define TIP_RISE            0.5f

void particle_spray_render(struct particle_spray* spray, struct render_batch* batch) {
    int particle_count = (int)(spray->total_time * (1.0f / CYCLE_TIME));
    int particle_offset = 0;

    if (particle_count > MAX_PARTICLE_COUNT) {
        particle_count = MAX_PARTICLE_COUNT;
    }

    if (spray->end_time != -1.0f) {
        particle_offset = (spray->total_time - spray->end_time) * (1.0f / CYCLE_TIME);

        if (particle_offset > particle_count) {
            return;
        }

        particle_count -= particle_offset;
    }

    struct render_batch_billboard_element* element = render_batch_add_particles(batch, spray->material, particle_count);

    float time_lerp = spray->cycle_time * (1.0f / CYCLE_TIME);

    for (int i = 0; i < element->sprite_count; i += 1) {
        struct render_billboard_sprite* sprite = &element->sprites[i];

        float particle_time = (i + particle_offset + time_lerp) * (1.0f / MAX_PARTICLE_COUNT);

        sprite->color.r = 255;
        sprite->color.g = 255;
        sprite->color.b = 255;
        sprite->color.a = 255;

        sprite->radius = particle_time * MAX_RADIUS;

        int final_index = i + spray->index_offset;

        if (final_index >= MAX_PARTICLE_COUNT) {
            final_index -= MAX_PARTICLE_COUNT;
        }

        vector3AddScaled(&spray->position, &spray->direction, particle_time * FIRE_LENGTH, &sprite->position);
        vector3AddScaled(&sprite->position, &spray->particle_offset[final_index], particle_time, &sprite->position);

        if (particle_time > START_FADE) {
            float alpha = 1.0f - (particle_time - START_FADE) * (1.0f / (1.0f - START_FADE));

            sprite->color.a = (uint8_t)(alpha * 255);
            sprite->position.y += TIP_RISE * (1.0f - alpha);
        }
    }
}


struct particle_spray* particle_spray_new() {
    struct particle_spray* result = effect_malloc(sizeof(struct particle_spray));

    result->cycle_time = 0.0f;
    result->total_time = 0.0f;
    result->end_time = -1.0f;

    result->index_offset = 0;

    for (int i = 0; i < MAX_PARTICLE_COUNT; i += 1) {
        result->particle_offset[i] = gZeroVec;

        struct Vector3* offset = &result->particle_offset[result->index_offset];
        offset->x = randomInRangef(-MAX_RANDOM_OFFSET, MAX_RANDOM_OFFSET);
        offset->y = randomInRangef(-MAX_RANDOM_OFFSET, MAX_RANDOM_OFFSET);
        offset->z = randomInRangef(-MAX_RANDOM_OFFSET, MAX_RANDOM_OFFSET);
    }

    return result;
}

void particle_spray_free(struct particle_spray* spray) {
    effect_free(spray);
}

void particle_spray_stop(struct particle_spray* spray) {
    spray->end_time = spray->total_time;
}

bool particle_spray_has_stopped(struct particle_spray* spray) {
    return spray->end_time != -1 && spray->total_time > spray->end_time + CYCLE_TIME * MAX_PARTICLE_COUNT;
}

void particle_spray_update(struct particle_spray* spray) {
    spray->cycle_time += fixed_time_step;
    spray->total_time += fixed_time_step;

    if (spray->cycle_time > CYCLE_TIME) {
        spray->cycle_time -= CYCLE_TIME;

        if (spray->index_offset == 0) {
            spray->index_offset = MAX_PARTICLE_COUNT - 1;
        } else {
            spray->index_offset -= 1;
        }
    }

    if (particle_spray_has_stopped(spray)) {
        particle_spray_free(spray);
    }
}