#include "lightning_effect.h"

#include "effect_allocator.h"
#include "../render/render_scene.h"
#include "../spell/assets.h"
#include "../render/defs.h"
#include "../math/mathf.h"

void lightning_effect_render(void* data, struct render_batch* batch) {
    struct lightning_effect* effect = (struct lightning_effect*)data;

    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    mat4x4 mtx;
    struct Vector3 scaled_pos;
    vector3Scale(&effect->start_position, &scaled_pos, WORLD_SCALE);
    matrixFromPosition(mtx, &scaled_pos);
    render_batch_relative_mtx(batch, mtx);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    render_batch_add_tmesh(batch, spell_assets_get()->lightning_mesh, mtxfp, 1, &effect->armature, NULL);
}

struct lightning_effect* lightning_effect_new(struct Vector3* position, struct lightning_effect_def* def) {

    struct lightning_effect* result = effect_malloc(sizeof(struct lightning_effect));
    armature_init(&result->armature, &spell_assets_get()->lightning_mesh->armature);
    render_scene_add(&result->last_position, 1.0f, lightning_effect_render, result);

    for (int i = 0; i < result->armature.bone_count; i += 1) {
        result->armature.pose[i].scale = gZeroVec;
    }

    result->def = def;
    result->next_transform = 0;
    result->start_position = *position;

    return result;
}

void lightning_effect_set_position(struct lightning_effect* effect, struct Vector3* position, struct Vector3* direction, float radius) {
    effect->last_position = *position;

    for (int i = 0; i < 2; i += 2) {
        struct Transform* next_transform = &effect->armature.pose[effect->next_transform];

        struct Vector3 relative_pos;
        vector3Sub(position, &effect->start_position, &relative_pos);

        vector3Scale(&relative_pos, &next_transform->position, MODEL_SCALE);
        vector3Scale(&gOneVec, &next_transform->scale, randomInRangef(radius * 0.5f, radius));

        struct Quaternion look_rotation;
        quatLook(direction, &gUp, &look_rotation);

        struct Quaternion look_offset;
        quatAxisAngle(&gUp, randomInRangef(-effect->def->spread, effect->def->spread), &look_offset);

        struct Quaternion roll_rotation;
        quatAxisAngle(&gForward, randomInRangef(0.0f, 6.28f), &roll_rotation);

        struct Quaternion combined_rotation;
        quatMultiply(&look_offset, &roll_rotation, &combined_rotation);

        quatMultiply(&look_rotation, &combined_rotation, &next_transform->rotation);

        effect->next_transform += 1;

        if (effect->next_transform == effect->armature.bone_count) {
            effect->next_transform = 0;
        }
    }
}

void lightning_effect_free(struct lightning_effect* effect) {
    render_scene_remove(effect);
    effect_free(effect);
}