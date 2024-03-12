#include "renderable.h"

#include "../resource/mesh_cache.h"

void renderable_init(struct renderable* renderable, struct Transform* transform, const char* mesh_filename) {
    renderable->transform = transform;
    renderable->mesh = mesh_cache_load(mesh_filename);
    armature_init(&renderable->armature, &renderable->mesh->armature);
}

void renderable_destroy(struct renderable* renderable) {
    mesh_cache_release(renderable->mesh);
    renderable->mesh = NULL;
}

void renderable_single_axis_init(struct renderable_single_axis* renderable, struct TransformSingleAxis* transform, const char* mesh_filename) {
    renderable->transform = transform;
    renderable->mesh = mesh_cache_load(mesh_filename);
    armature_init(&renderable->armature, &renderable->mesh->armature);
}

void renderable_single_axis_destroy(struct renderable_single_axis* renderable) {
    mesh_cache_release(renderable->mesh);
    renderable->mesh = NULL;
}