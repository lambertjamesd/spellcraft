#include "scale_in_fade_out.h"

#include "../time/time.h"
#include "../render/render_scene.h"
#include "../render/render_batch.h"
#include "../render/defs.h"

#include "effect_allocator.h"

#define SCALE_IN_TIME   0.5f
#define FADE_OUT_TIME   1.0f

void scale_in_fade_out_render(void* data, struct render_batch* batch) {
    struct scale_in_fade_out* effect = (struct scale_in_fade_out*)data;

    float alpha;

    if (effect->end_time) {
        alpha = 1.0f - (game_time - effect->end_time) * (1.0f / FADE_OUT_TIME);
    } else {
        alpha = 1.0f;
    }

    if (alpha < 0.0f) {
        return;
    }

    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    float scale = (game_time - effect->start_time) * (1.0f / SCALE_IN_TIME);

    if (scale > 1.0f) {
        scale = 1.0f;
    }

    mat4x4 mtx;
    transformSAToMatrix(&effect->transform, mtx, effect->radius * scale);
    mtx[3][0] *= SCENE_SCALE;
    mtx[3][1] *= SCENE_SCALE;
    mtx[3][2] *= SCENE_SCALE;
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    struct render_batch_element* element = render_batch_add_tmesh(batch, effect->mesh, mtxfp, 1, NULL, NULL);

    element->mesh.color.a = (uint8_t)(255.0f * alpha);
}

void scale_in_fade_out_free(struct scale_in_fade_out* effect);

void scale_in_fade_out_update(void* data) {
    struct scale_in_fade_out* effect = (struct scale_in_fade_out*)data;

    if (game_time - effect->end_time > FADE_OUT_TIME) {
        scale_in_fade_out_free(effect);
    }
}

void scale_in_fade_out_free(struct scale_in_fade_out* effect) {
    update_remove(effect);
    render_scene_remove(effect);
    effect_free(effect);
}

struct scale_in_fade_out* scale_in_fade_out_new(struct tmesh* mesh, struct Vector3* pos, struct Vector2* rotation, float radius) {
    struct scale_in_fade_out* result = effect_malloc(sizeof(struct scale_in_fade_out));

    result->start_time = game_time;
    result->end_time = 0.0f;

    result->transform.position = *pos;
    result->transform.rotation = *rotation;
    result->radius = radius;

    result->mesh = mesh;

    render_scene_add(&result->transform.position, radius, scale_in_fade_out_render, result);

    return result;
}

void scale_in_fade_out_set_transform(struct scale_in_fade_out* effect, struct Vector3* pos, struct Vector2* rotation) {
    effect->transform.position = *pos;
    effect->transform.rotation = *rotation;
}

void scale_in_fade_out_stop(struct scale_in_fade_out* effect) {
    effect->end_time = game_time;
    // TODO possibly implement a timer callback system
    update_add(effect, scale_in_fade_out_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);
}