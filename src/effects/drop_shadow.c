#include "drop_shadow.h"

#include "../render/render_scene.h"
#include "../resource/tmesh_cache.h"

void drop_shadow_render(void* data, struct render_batch* batch) {
    struct drop_shadow* drop_shadow = (struct drop_shadow*)data;

    struct contact* contact = dynamic_object_get_ground(drop_shadow->target);

    if (!contact) {
        return;
    }

    struct TransformSingleAxis transform;
    transform.position = contact->point;
    transform.rotation = gRight2;
    // quatLook(&gRight, &contact->normal, &transform.rotation);
    T3DMat4FP* mtx = render_batch_transformfp_from_sa(batch, &transform, 1.0f);
    render_batch_add_tmesh(batch, drop_shadow->mesh, mtx, 1, NULL, NULL);
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