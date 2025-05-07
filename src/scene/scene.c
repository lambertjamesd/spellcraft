#include "scene.h"

#include "../render/render_batch.h"
#include "../overworld/overworld_load.h"

struct scene* current_scene;

static char next_scene_name[64];
static char next_entrance_name[16];

void scene_render(void* data, struct render_batch* batch) {
    struct scene* scene = (struct scene*)data;

    for (int i = 0; i < scene->static_entity_count; ++i) {
        render_batch_add_tmesh(batch, &scene->static_entities[i].tmesh, NULL, 0, NULL, NULL);
    }
}

void scene_update(void* data) {
    struct scene* scene = (struct scene*)data;

    struct Vector3 player_center = scene->player.cutscene_actor.transform.position;
    player_center.y += scene->player.cutscene_actor.collider.center.y;

    for (int i = 0; i < scene->loading_zone_count; i += 1) {
        if (box3DContainsPoint(&scene->loading_zones[i].bounding_box, &player_center)) {
            scene_queue_next(scene->loading_zones[i].scene_name);
        }
    }

    if (scene->overworld) {
        overworld_check_loaded_tiles(scene->overworld);
        overworld_check_collider_tiles(scene->overworld, player_get_position(&scene->player));
    }
}

void scene_queue_next(char* scene_name) {
    char* curr = scene_name;
    char* out = next_scene_name;
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

void scene_clear_next() {
    next_scene_name[0] = '\0';
}

bool scene_has_next() {
    return next_scene_name[0] != 0;
}

char* scene_get_next() {
    return next_scene_name;
}

char* scene_get_next_entry() {
    return next_entrance_name;
}

struct named_location* scene_find_location(char* name) {
    for (int i = 0; i < current_scene->named_location_count; i += 1) {
        if (strcmp(name, current_scene->named_locations[i].name) == 0) {
            return &current_scene->named_locations[i];
        }
    }

    return NULL;
}

void scene_entity_apply_types(void* definition, char* string_table, struct entity_field_type_location* type_locations, int type_location_count) {
    for (int i = 0; i < type_location_count; i += 1) {
        switch (type_locations[i].type) {
            case ENTITY_FIELD_TYPE_STRING: {
                char** entry_location = (char**)((char*)definition + type_locations[i].offset);
                *entry_location += (int)string_table;
                break;
            }
        }
    }
}
