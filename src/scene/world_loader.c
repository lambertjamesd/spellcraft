#include "world_loader.h"

#include <malloc.h>
#include <string.h>
#include <libdragon.h>
#include "../resource/mesh_collider.h"
#include "../resource/tmesh_cache.h"
#include "../render/render_scene.h"
#include "../time/time.h"

#include "../enemies/biter.h"

#include "../npc/npc.h"

#include "../objects/collectable.h"
#include "../objects/crate.h"
#include "../objects/ground_torch.h"
#include "../objects/training_dummy.h"
#include "../objects/treasure_chest.h"

#include "../collision/collision_scene.h"

static struct entity_definition world_entity_definitions[] = {
    ENTITY_DEFINITION(biter),
    ENTITY_DEFINITION(collectable),
    ENTITY_DEFINITION(crate),
    ENTITY_DEFINITION(ground_torch),
    ENTITY_DEFINITION(npc),
    ENTITY_DEFINITION(training_dummy),
    ENTITY_DEFINITION(treasure_chest),
};

// WRLD
#define EXPECTED_HEADER 0x57524C44

struct entity_definition* world_find_def(const char* name) {
   for (int i = 0; i < sizeof(world_entity_definitions) / sizeof(*world_entity_definitions); i += 1) {
        struct entity_definition* def = &world_entity_definitions[i];

        if (strcmp(name, def->name) == 0) {
            return def;
        }
   }

   return NULL;
}

struct type_location {
    uint8_t type;
    uint8_t offset;
};

enum type_location_types {
    TYPE_LOCATION_STRING,
};

void world_apply_types(void* definition, char* string_table, struct type_location* type_locations, int type_location_count) {
    for (int i = 0; i < type_location_count; i += 1) {
        switch (type_locations[i].type) {
            case TYPE_LOCATION_STRING: {
                char** entry_location = (char**)((char*)definition + type_locations[i].offset);
                *entry_location += (int)string_table;
                break;
            }
        }
    }
}

void world_load_entity(struct world* world, struct entity_data* entity_data, FILE* file) {
    uint8_t name_len;
    fread(&name_len, 1, 1, file);
    char name[name_len + 1];
    fread(name, 1, name_len, file);
    name[name_len] = '\0';

    struct entity_definition* def = world_find_def(name);

    assert(def);

    fread(&entity_data->entity_count, 2, 1, file);
    uint16_t definition_size;
    fread(&definition_size, 2, 1, file);
    assert(definition_size == def->definition_size);

    uint8_t type_location_count;

    fread(&type_location_count, 1, 1, file);

    struct type_location type_locations[type_location_count];
    fread(type_locations, sizeof(struct type_location), type_location_count, file);

    char* entity = malloc(def->entity_size * entity_data->entity_count);
    char entity_def_data[definition_size * entity_data->entity_count];
    char* entity_def = entity_def_data;

    entity_data->definition = def;
    entity_data->entities = entity;

    fread(entity_def_data, definition_size, entity_data->entity_count, file);

    for (int entity_index = 0; entity_index < entity_data->entity_count; entity_index += 1) {
        world_apply_types(entity_def, world->string_table, type_locations, type_location_count);
        def->init(entity, entity_def);

        entity += def->entity_size;
        entity_def += def->definition_size;
    }
}

void world_destroy_entity(struct entity_data* entity_data) {
    char* entity = entity_data->entities;

    for (int entity_index = 0; entity_index < entity_data->entity_count; entity_index += 1) {
        entity_data->definition->destroy(entity);
        entity += entity_data->definition->entity_size;
    }

    free(entity_data->entities);
}

struct world* world_load(const char* filename) {
    FILE* file = asset_fopen(filename, NULL);

    int header;
    fread(&header, 1, 4, file);
    assert(header == EXPECTED_HEADER);

    struct world* world = malloc(sizeof(struct world));

    struct player_definition player_def;
    player_def.location = gZeroVec;
    player_def.rotation = gRight2;

    uint8_t location_count;
    fread(&location_count, 1, 1, file);

    bool found_entry = false;
    
    for (int i = 0; i < location_count; i += 1) {
        uint8_t name_length;
        fread(&name_length, 1, 1, file);
        char name[name_length + 1];
        fread(name, name_length, 1, file);
        name[name_length] = '\0';

        struct Vector3 pos;
        struct Vector2 rot;

        fread(&pos, sizeof(struct Vector3), 1, file);
        fread(&rot, sizeof(struct Vector2), 1, file);

        if (found_entry) {
            continue;
        }

        if (strcmp(name, "default") == 0) {
            player_def.location = pos;
            player_def.rotation = rot;
        }

        if (strcmp(name, world_get_next_entry()) == 0) {
            player_def.location = pos;
            player_def.rotation = rot;
            found_entry = true;
        }
    }

    inventory_init();
    camera_init(&world->camera, 70.0f, 1.0f, 100.0f);
    player_init(&world->player, &player_def, &world->camera.transform);
    camera_controller_init(&world->camera_controller, &world->camera, &world->player);

    pause_menu_init(&world->pause_menu);
    hud_init(&world->hud, &world->player);

    uint16_t static_count;
    fread(&static_count, 2, 1, file);

    world->static_entities = malloc(sizeof(struct static_entity) * static_count);
    world->static_entity_count = static_count;

    for (int i = 0; i < static_count; ++i) {
        uint8_t str_len;
        fread(&str_len, 1, 1, file);

        struct static_entity* entity = &world->static_entities[i];
        tmesh_load(&world->static_entities[i].tmesh, file);
    }

    mesh_collider_load(&world->mesh_collider, file);
    collision_scene_use_static_collision(&world->mesh_collider);

    uint16_t strings_length;
    fread(&strings_length, 2, 1, file);

    world->string_table = malloc(strings_length);
    fread(world->string_table, strings_length, 1, file);

    fread(&world->entity_data_count, 2, 1, file);

    world->entity_data = malloc(sizeof(struct entity_data) * world->entity_data_count);

    for (int i = 0; i < world->entity_data_count; i += 1) {
        world_load_entity(world, &world->entity_data[i], file);
    }

    fread(&world->loading_zone_count, 2, 1, file);

    world->loading_zones = malloc(sizeof(struct loading_zone) * world->loading_zone_count);
    fread(world->loading_zones, sizeof(struct loading_zone), world->loading_zone_count, file);

    for (int i = 0; i < world->loading_zone_count; i += 1) {
        world->loading_zones[i].world_name += (int)world->string_table;
    }

    fclose(file);

    render_scene_add(NULL, 0.0f, world_render, world);
    update_add(world, world_update, UPDATE_PRIORITY_CAMERA, UPDATE_LAYER_WORLD);

    return world;
}

void world_release(struct world* world) {
    if (!world) {
        return;
    }

    for (int i = 0; i < world->static_entity_count; ++i) {
        struct static_entity* entity = &world->static_entities[i];
        tmesh_release(&entity->tmesh);
    }
    free(world->static_entities);

    render_scene_remove(world);
    update_remove(world);

    pause_menu_destroy(&world->pause_menu);
    hud_destroy(&world->hud);
    player_destroy(&world->player);
    camera_controller_destroy(&world->camera_controller);

    inventory_destroy();

    collision_scene_remove_static_collision(&world->mesh_collider);
    mesh_collider_release(&world->mesh_collider);

    for (int i = 0; i < world->entity_data_count; i += 1) {
        world_destroy_entity(&world->entity_data[i]);
    }
    free(world->entity_data);

    free(world->string_table);
    free(world->loading_zones);

    free(world);
}