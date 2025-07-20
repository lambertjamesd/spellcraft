#include "hit_effect.h"

#include "effect_allocator.h"
#include "../time/time.h"
#include "../render/render_scene.h"
#include "../math/mathf.h"

#define RADIUS  1.25f
#define SCALE_IN_TIME 0.125f
#define EFFECT_TIME 0.5f
#define FADE_OUT_TIME 0.25f

#include "./assets.h"

void hit_effect_render(void* data, struct render_batch* batch) {
    struct hit_effect* effect = (struct hit_effect*)data;

    float scale;

    if (effect->timer < SCALE_IN_TIME) {
        scale = effect->timer * (RADIUS / SCALE_IN_TIME);
        scale *= scale;
    } else {
        scale = RADIUS;
    }
    
    u_int8_t alpha;

    if (effect->timer < (EFFECT_TIME - FADE_OUT_TIME)) {
        alpha = 255;
    } else {
        alpha = (uint8_t)((EFFECT_TIME - effect->timer) * (255.0f / FADE_OUT_TIME));
    }

    vector3Scale(&gOneVec, &effect->transform.scale, scale);

    T3DMat4FP* mtxfp = render_batch_transformfp_from_full(batch, &effect->transform);

    if (!mtxfp) {
        return;
    }

    struct render_batch_element* element = render_batch_add_tmesh(batch, effect_assets_get()->hit_effect, mtxfp, 1, NULL, NULL);
    element->mesh.color = (color_t){0, 255, 0, alpha};
    element->mesh.use_prim_color = 1;
}

void hit_effect_update(void* data) {
    struct hit_effect* effect = (struct hit_effect*)data;

    effect->timer += fixed_time_step;

    if (effect->timer > EFFECT_TIME) {
        update_remove(effect);
        render_scene_remove(effect);
    }
}

void hit_effect_start(struct Vector3* position, struct Vector3* normal) {
    struct hit_effect* result = effect_malloc(sizeof(struct hit_effect));

    if (!result) {
        return;
    }

    result->timer = 0.0f;
    result->transform.position = *position;
    struct Vector3 up = {randomInRangef(-1.0f, 1.0f), randomInRangef(-1.0f, 1.0f), randomInRangef(-1.0f, 1.0f)};
    quatLook(normal, &up, &result->transform.rotation);
    vector3Scale(&gOneVec, &result->transform.scale, RADIUS);

    update_add(result, hit_effect_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);
    render_scene_add(&result->transform.position, 0.5f, hit_effect_render, result);
}