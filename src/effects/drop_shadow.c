#include "drop_shadow.h"

#include "../render/render_scene.h"
#include "../resource/tmesh_cache.h"
#include "../math/matrix.h"
#include "../render/defs.h"
#include "../collision/collision_scene.h"

#define SHADOW_SCALE    (0.5f * MODEL_WORLD_SCALE)

void drop_shadow_render(void* data, struct render_batch* batch) {
    struct drop_shadow* drop_shadow = (struct drop_shadow*)data;

    struct contact* contact = dynamic_object_get_ground(drop_shadow->target);
    struct Vector3 pos;
    struct Vector3 normal;

    if (!contact) {
        struct mesh_shadow_cast_result cast_result;
        if (!collision_scene_shadow_cast(drop_shadow->target->position, &cast_result)) {
            return;
        }
        pos = *drop_shadow->target->position;
        pos.y = cast_result.y;
        normal = cast_result.normal;
    } else {
        pos = contact->point;
        normal = contact->normal;
    }

    if (normal.y < 0.0001f) {
        return;
    }
    
    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    struct Vector3 skewScale;
    vector3Scale(&normal, &skewScale, SHADOW_SCALE / normal.y);

    mat4x4 mtx;
    matrixFromScale(mtx, SHADOW_SCALE);
    mtx[0][1] = -skewScale.x;
    mtx[2][1] = -skewScale.z;
    matrixApplyScaledPos(mtx, &pos, WORLD_SCALE);
    render_batch_relative_mtx(batch, mtx);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    render_batch_add_tmesh(batch, drop_shadow->mesh, mtxfp, 1, NULL, NULL);
}

void drop_shadow_init(struct drop_shadow* drop_shadow, struct dynamic_object* target) {
    drop_shadow->target = target;

    render_scene_add(target->position, 1.0f, drop_shadow_render, drop_shadow);
    drop_shadow->mesh = tmesh_cache_load("rom:/meshes/effects/drop-shadow.tmesh");
}

void drop_shadow_destroy(struct drop_shadow* drop_shadow) {
    render_scene_remove(drop_shadow);
    tmesh_cache_release(drop_shadow->mesh);
}