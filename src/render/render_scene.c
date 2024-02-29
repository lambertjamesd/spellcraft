#include "render_scene.h"

#include "../util/blist.h"
#include <malloc.h>
#include <stdbool.h>

#define MIN_RENDER_SCENE_SIZE   64

struct render_scene r_scene_3d;

void render_scene_reset(struct render_scene* scene) {
    callback_list_reset(&scene->callbacks, sizeof(struct render_scene_element), MIN_RENDER_SCENE_SIZE, NULL);
}

render_id render_scene_add(struct render_scene* scene, struct Vector3* center, float radius, render_scene_callback callback, void* data) {
    struct render_scene_element element;

    element.data = data;
    element.center = center;
    element.radius = radius;

    return callback_list_insert(&scene->callbacks, callback, &element);
}

void render_scene_render_renderable(void* data, struct render_batch* batch) {
    struct renderable* renderable = (struct renderable*)data;

    mat4x4* mtx = render_batch_get_transform(batch);

    if (!mtx) {
        return;
    }

    transformToMatrix(renderable->transform, *mtx);

    render_batch_add_mesh(batch, renderable->mesh, mtx);
}

render_id render_scene_add_renderable(struct render_scene* scene, struct renderable* renderable, float radius) {
    return render_scene_add(scene, &renderable->transform->position, radius, render_scene_render_renderable, renderable);
}

void render_scene_remove(struct render_scene* scene, render_id id) {
    callback_list_remove(&scene->callbacks, id);
}

void render_scene_render(struct render_scene* scene, struct Camera* camera, struct render_viewport* viewport) {
    struct render_batch batch;

    struct ClippingPlanes clipping_planes;
    mat4x4 view_proj_matrix;

    float aspect_ratio = (float)viewport->w / (float)viewport->h;

    camera_apply(camera, aspect_ratio, &clipping_planes, view_proj_matrix);

    render_batch_init(&batch);

    struct callback_element* current = callback_list_get(&scene->callbacks, 0);

    for (int i = 0; i < scene->callbacks.count; ++i) {
        struct render_scene_element* el = callback_element_get_data(current);
        ((render_scene_callback)current->callback)(el->data, &batch);

        current = callback_list_next(&scene->callbacks, current);
    }
    render_batch_finish(&batch, view_proj_matrix, viewport);
}