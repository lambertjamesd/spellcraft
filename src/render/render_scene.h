#ifndef __RENDER_RENDER_SCENE_H__
#define __RENDER_RENDER_SCENE_H__

#include "../math/vector3.h"
#include "./renderable.h"
#include "./render_batch.h"

typedef void (*render_scene_callback)(void* data, struct render_batch* batch);

typedef int render_id;

struct render_scene_element {
    struct Vector3* center;
    float radius;
    render_scene_callback callback;
    void* data;
    render_id id;
};

struct render_scene {
    render_id next_id;
    struct render_scene_element* elements;
    short element_count;
    short element_capacity;
};

void render_scene_init(struct render_scene* scene);

render_id render_scene_add(struct render_scene* scene, struct Vector3* center, float radius, render_scene_callback callback, void* data);
render_id render_scene_add_renderable(struct render_scene* scene, struct renderable* renderable, float radius);
void render_scene_remove(render_id id);

#endif