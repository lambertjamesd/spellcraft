#include "world.h"

#include "../render/render_batch.h"

static char next_world_name[64];
static char next_entrance_name[16];

void world_render(void* data, struct render_batch* batch) {
    struct world* world = (struct world*)data;

    for (int i = 0; i < world->static_entity_count; ++i) {
        render_batch_add_tmesh(batch, &world->static_entities[i].tmesh, NULL, NULL);
    }
}

void world_update(void* data) {
    struct world* world = (struct world*)data;

    struct Vector3 player_center = world->player.transform.position;
    player_center.y += world->player.collision.center.y;

    for (int i = 0; i < world->loading_zone_count; i += 1) {
        if (box3DContainsPoint(&world->loading_zones[i].bounding_box, &player_center)) {
            world_queue_next(world->loading_zones[i].world_name);
        }
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

    // skip the bound symbol
    ++curr;

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

char* world_get_next_entry() {
    return next_entrance_name;
}