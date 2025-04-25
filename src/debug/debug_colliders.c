#include "debug_colliders.h"

#include "../render/tmesh.h"
#include "../collision/collision_scene.h"
#include "../render/render_scene.h"
#include "../resource/tmesh_cache.h"
#include "../scene/scene.h"
#include "../render/defs.h"

static struct tmesh* bb_mesh;

void debug_render_colliders(void* data, struct render_batch* batch) {
    for (int i = 0; i < collision_scene_get_count(); i += 1) {
        struct dynamic_object* dynamic_object = collision_scene_get_element(i);

        T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

        if (!mtxfp) {
            return;
        }

        mat4x4 mtx;

        matrixFromScale(mtx, MODEL_WORLD_SCALE);
        mtx[0][0] = (dynamic_object->bounding_box.max.x - dynamic_object->bounding_box.min.x) * MODEL_WORLD_SCALE;
        mtx[1][1] = (dynamic_object->bounding_box.max.y - dynamic_object->bounding_box.min.y) * MODEL_WORLD_SCALE;
        mtx[2][2] = (dynamic_object->bounding_box.max.z - dynamic_object->bounding_box.min.z) * MODEL_WORLD_SCALE;
        matrixApplyScaledPos(mtx, &dynamic_object->bounding_box.min, WORLD_SCALE);
        render_batch_relative_mtx(batch, mtx);
        t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);
        
        render_batch_add_tmesh(batch, bb_mesh, mtxfp, 1, NULL, NULL);
    }
}

void debug_collider_enable() {
    if (bb_mesh) {
        return;
    }
    bb_mesh = tmesh_cache_load("rom:/meshes/test/bounding_box.tmesh");
    render_scene_add(NULL, 0.0f, debug_render_colliders, bb_mesh);
}

void debug_collider_disable() {
    if (bb_mesh) {
        return;
    }

    render_scene_remove(bb_mesh);
    tmesh_cache_release(bb_mesh);
    bb_mesh = NULL;
}