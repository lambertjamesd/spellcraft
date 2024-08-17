#include "burning_effect.h"

#include "./effect_allocator.h"
#include "../render/render_scene.h"
#include "../render/defs.h"
#include "../time/time.h"
#include "../spell/assets.h"
#include "../math/mathf.h"
#include <assert.h>

void burning_effect_update(void* data) {
    struct burning_effect* effect = (struct burning_effect*)data;

    if (effect->current_size == 0.0f && effect->current_time <= 0.0f && effect->size == 0.0f) {
        update_remove(effect);
        render_scene_remove(effect);
        effect_free(effect);
        return;
    }

    if (effect->current_time > 0.0f) {
        effect->current_time -= fixed_time_step;
        effect->current_size = mathfMoveTowards(effect->current_size, effect->size, 1.0f * fixed_time_step);
    } else {
        effect->current_size = mathfMoveTowards(effect->current_size, 0.0f, effect->size ? 0.25f * fixed_time_step : 4.0f * fixed_time_step);
    }
}

void burning_effect_render(void* data, struct render_batch* batch) {
    struct burning_effect* effect = (struct burning_effect*)data;

    struct Vector3 flame_position = effect->position;
    vector3Scale(&flame_position, &flame_position, SCENE_SCALE);
        
    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    mat4x4 mtx;
    memcpy(mtx, &batch->camera_matrix, sizeof(mat4x4));
    matrixApplyPosition(mtx, &flame_position);
    matrixApplyScale(mtx, effect->current_size);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    render_batch_add_tmesh(batch, spell_assets_get()->flame_mesh, mtxfp, NULL);
}

struct burning_effect* burning_effect_new(struct Vector3* position, float size, float duration) {
    struct burning_effect* result = effect_malloc(sizeof(struct burning_effect));
    result->position = *position;
    result->size = size;

    assert(size > 0.0f);

    render_scene_add(&result->position, size, burning_effect_render, result);
    result->current_time = duration;
    result->current_size = 0.0f;

    update_add(result, burning_effect_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);

    return result;
}

void bunring_effect_set_position(struct burning_effect* effect, struct Vector3* position) {
    effect->position = *position;
}

void burning_effect_refresh(struct burning_effect* effect, float duration) {
    effect->current_time = duration;
}

void burning_effect_stop(struct burning_effect* effect) {
    effect->current_time = 0.0f;
}

void burning_effect_free(struct burning_effect* effect) {
    effect->size = 0.0f;
    effect->current_time = 0.0f;
}

bool burning_effect_is_active(struct burning_effect* effect) {
    return effect->current_size > 0.0f || effect->current_time > 0.0f;
}