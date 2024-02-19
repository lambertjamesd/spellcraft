#include "render_scene.h"

#define MIN_RENDER_SCENE_SIZE   64

void render_scene_init(struct render_scene* scene) {
    scene->next_id = 1;
}

render_id render_scene_add(struct render_scene* scene, struct Vector3* center, float radius, render_scene_callback callback, void* data) {
    render_id result = scene->next_id;
    scene->next_id += 1;
    return result;
}

void render_scene_render_renderable(void* data, struct render_batch* batch) {
    struct renderable* renderable = (struct renderable*)data;

    mat4x4* mtx = render_batch_get_transform(batch);

    if (!mtx) {
        return;
    }

    transformToMatrix(&renderable->transform, *mtx);

    render_batch_add_mesh(batch, renderable->mesh, mtx);
}

render_id render_scene_add_renderable(struct render_scene* scene, struct renderable* renderable, float radius) {
    return render_scene_add(scene, &renderable->transform.position, radius, render_scene_render_renderable, renderable);
}

void render_scene_remove(render_id id) {

}