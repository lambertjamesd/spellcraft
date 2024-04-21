#include "world.h"

#include "../render/render_batch.h"

static char next_world_name[64];
static char next_entrance_name[16];

void world_render(void* data, struct render_batch* batch) {
    struct world* world = (struct world*)data;
    for (int i = 0; i < world->static_entity_count; ++i) {
        render_batch_add_mesh(batch, world->static_entities[i].mesh, NULL, NULL);
    }
}

void world_queue_next(char* world_name) {
    char* curr = world_name;
    char* out = next_world_name;
    while (*curr && *curr != '#') {
        *out++ = *curr++;
    }

    *out = '\0';

    out = next_entrance_name;

    if (!*curr) {
        next_entrance_name[0] = '\0';
        return;
    }

    while ((*out++ = *curr++));
}

void world_clear_next() {
    next_world_name[0] = '\0';
}

bool world_has_next() {
    return next_world_name[0] != 0;
}

char* world_get_next() {
    return next_world_name;
}