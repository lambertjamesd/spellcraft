#include "mana_gem.h"
#include <malloc.h>
#include "assets.h"

#include "../render/defs.h"
#include "../render/render_scene.h"
#include "../math/mathf.h"

#define RADIUS_SCALE    0.02f
#define MIN_RADIUS      0.1f

void mana_gem_render(void* data, struct render_batch* batch) {
    struct mana_gem* gem = (struct mana_gem*)data;

    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    mat4x4 mtx;
    transformSAToMatrix(&gem->transform, mtx, gem->radius);
    render_batch_relative_mtx(batch, mtx);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    render_batch_add_tmesh(
        batch, 
        object_assets_get()->mana_gem_mesh,
        mtxfp,
        1,
        NULL,
        NULL
    );
}

struct mana_gem* mana_gem_new(struct Vector3* position, float mana_amount) {
    struct mana_gem* result = malloc(sizeof(struct mana_gem));
    mana_gem_init(result, position, mana_amount);
    return result;
}

void mana_gem_free(struct mana_gem* gem) {
    mana_gem_destroy(gem);
    free(gem);
}

void mana_gem_init(struct mana_gem* gem, struct Vector3* position, float mana_amount) {
    gem->transform.position = *position;
    gem->transform.rotation = gRight2;
    renderable_single_axis_init_direct(&gem->renderable, &gem->transform, object_assets_get()->mana_gem_mesh);
    gem->radius = maxf(sqrtf(mana_amount) * RADIUS_SCALE, MIN_RADIUS);
    render_scene_add(&gem->transform.position, gem->radius, mana_gem_render, gem);
}

void mana_gem_destroy(struct mana_gem* gem) {
    render_scene_remove(&gem->renderable);
    renderable_destroy_direct(&gem->renderable);
}