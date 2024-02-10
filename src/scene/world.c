#include "world.h"

#include "../render/render_batch.h"

void world_render(struct world* world) {
    struct render_batch batch;
    render_batch_init(&batch);

    for (int i = 0; i < world->static_entity_count; ++i) {
        render_batch_add_mesh(&batch, world->static_entities[i].mesh, NULL);
    }

    render_batch_finish(&batch);
}