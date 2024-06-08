#include "renderable.h"

#include "../resource/tmesh_cache.h"

void renderable_init(struct renderable* renderable, struct Transform* transform, const char* mesh_filename) {
    renderable->transform = transform;
    renderable->mesh = tmesh_cache_load(mesh_filename);
    armature_init(&renderable->armature, NULL);
}

void renderable_destroy(struct renderable* renderable) {
    tmesh_cache_release(renderable->mesh);
    renderable->mesh = NULL;
}

void renderable_single_axis_init(struct renderable_single_axis* renderable, struct TransformSingleAxis* transform, const char* mesh_filename) {
    renderable->transform = transform;
    renderable->mesh = tmesh_cache_load(mesh_filename);
    armature_init(&renderable->armature, NULL);
}

void renderable_single_axis_destroy(struct renderable_single_axis* renderable) {
    tmesh_cache_release(renderable->mesh);
    renderable->mesh = NULL;
}