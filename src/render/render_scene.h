#ifndef __RENDER_RENDER_SCENE_H__
#define __RENDER_RENDER_SCENE_H__

#include "../math/vector3.h"
#include "renderable.h"
#include "render_batch.h"
#include "camera.h"
#include "viewport.h"
#include "../util/callback_list.h"

typedef void (*render_scene_callback)(void* data, struct render_batch* batch);

typedef int render_id;

struct render_scene_element {
    void* data;
    struct Vector3* center;
    float radius;
};

struct render_scene {
    struct callback_list callbacks;
};

void render_scene_reset(struct render_scene* scene);

void render_scene_add(struct render_scene* scene, struct Vector3* center, float radius, render_scene_callback callback, void* data);
void render_scene_add_renderable(struct render_scene* scene, struct renderable* renderable, float radius);
void render_scene_add_renderable_single_axis(struct render_scene* scene, struct renderable_single_axis* renderable, float radius);
void render_scene_remove(struct render_scene* scene, void* data);

void render_scene_render(struct render_scene* scene, struct Camera* camera, struct render_viewport* viewport);

extern struct render_scene r_scene_3d;

#endif