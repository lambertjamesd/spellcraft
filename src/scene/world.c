#include "world.h"

#include "../render/render_batch.h"

void world_render(void* data, struct render_batch* batch) {
    struct world* world = (struct world*)data;
    for (int i = 0; i < world->static_entity_count; ++i) {
        render_batch_add_mesh(batch, world->static_entities[i].mesh, NULL);
    }
}