#include "living_sprite.h"

#include "../render/render_scene.h"
#include "../render/defs.h"
#include "assets.h"

void living_sprite_render(void* data, struct render_batch* batch) {
    struct living_sprite* living_sprite = (struct living_sprite*)data;

    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    struct Vector3 position;
    vector3Scale(&living_sprite->transform.position, &position, SCENE_SCALE);

    mat4x4 mtx;
    memcpy(mtx, &batch->camera_matrix, sizeof(mat4x4));
    matrixApplyPosition(mtx, &position);
    matrixApplyScale(mtx, 1.0f);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    render_batch_add_tmesh(batch, spell_assets_get()->fire_sprite, mtxfp, 1, NULL, NULL);
}

void living_sprite_init(struct living_sprite* living_sprite, struct spell_data_source* source, struct spell_event_options event_options) {
    vector3AddScaled(&source->position, &source->direction, 0.5f, &living_sprite->transform.position);
    living_sprite->transform.rotation = gRight2;

    render_scene_add(&living_sprite->transform.position, 0.5f, living_sprite_render, living_sprite);
}

bool living_sprite_update(struct living_sprite* living_sprite, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    return true;
}

void living_sprite_destroy(struct living_sprite* living_sprite) {
    render_scene_remove(living_sprite);
}