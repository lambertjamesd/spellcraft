#include "renderable.h"

#include "../resource/tmesh_cache.h"
#include <stddef.h>

void renderable_init(struct renderable* renderable, struct Transform* transform, const char* mesh_filename) {
    renderable->transform = transform;
    renderable->mesh = tmesh_cache_load(mesh_filename);
    renderable->force_material = NULL;
    armature_init(&renderable->armature, &renderable->mesh->armature);

    if (renderable->mesh->attatchment_count) {
        renderable->attachments = malloc(sizeof(struct tmesh*) * renderable->mesh->attatchment_count);
        memset(renderable->attachments, 0, sizeof(struct tmesh*) * renderable->mesh->attatchment_count);
    } else {
        renderable->attachments = NULL;
    }
}

void renderable_destroy(struct renderable* renderable) {
    tmesh_cache_release(renderable->mesh);
    free(renderable->attachments);
    renderable->mesh = NULL;
}

void renderable_single_axis_init(struct renderable_single_axis* renderable, struct TransformSingleAxis* transform, const char* mesh_filename) {
    renderable->transform = transform;
    renderable->mesh = tmesh_cache_load(mesh_filename);
    renderable->force_material = NULL;
    armature_init(&renderable->armature, &renderable->mesh->armature);

    if (renderable->mesh->attatchment_count) {
        renderable->attachments = malloc(sizeof(struct tmesh*) * renderable->mesh->attatchment_count);
        memset(renderable->attachments, 0, sizeof(struct tmesh*) * renderable->mesh->attatchment_count);
    } else {
        renderable->attachments = NULL;
    }
}

void renderable_single_axis_destroy(struct renderable_single_axis* renderable) {
    tmesh_cache_release(renderable->mesh);
    free(renderable->attachments);
    renderable->mesh = NULL;
}