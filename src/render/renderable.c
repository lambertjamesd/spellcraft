#include "renderable.h"

#include "../resource/mesh_cache.h"

void renderable_init(struct renderable* renderable, const char* mesh_filename) {
    transformInitIdentity(&renderable->transform);
    renderable->mesh = mesh_cache_load("rom:/meshes/player/player.mesh");
}

void renderable_destroy(struct renderable* renderable) {
    mesh_cache_release(renderable->mesh);
    renderable->mesh = NULL;
}