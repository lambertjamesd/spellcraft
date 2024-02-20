#include "renderable.h"

#include "../resource/mesh_cache.h"

void renderable_init(struct renderable* renderable, struct Transform* transform, const char* mesh_filename) {
    renderable->transform = transform;
    renderable->mesh = mesh_cache_load("rom:/meshes/player/player.mesh");
}

void renderable_destroy(struct renderable* renderable) {
    mesh_cache_release(renderable->mesh);
    renderable->mesh = NULL;
}