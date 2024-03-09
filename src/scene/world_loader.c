#include "world_loader.h"

#include <malloc.h>
#include <string.h>
#include <libdragon.h>
#include "../resource/mesh_cache.h"
#include "../resource/mesh_collider.h"
#include "../render/render_scene.h"

#include "../objects/crate.h"

#include "../collision/collision_scene.h"

static struct entity_definition world_entity_definitions[] = {
    {"crate", (entity_init)crate_init, (entity_destroy)crate_destroy, sizeof(struct crate), sizeof(struct crate_definition)},
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

    char* entity = malloc(def->entity_size * entity_data->entity_count);
    char entity_def_data[definition_size * entity_data->entity_count];
    char* entity_def = entity_def_data;

    entity_data->definition = def;
    entity_data->entities = entity;

    fread(entity_def_data, definition_size, entity_data->entity_count, file);

    for (int entity_index = 0; entity_index < entity_data->entity_count; entity_index += 1) {
        def->init(entity, entity_def_data);

        entity += def->entity_size;
        entity_def += def->definition_size;
    }
}

struct world* world_load(const char* filename) {
    FILE* file = asset_fopen(filename, NULL);

    int header;
    fread(&header, 1, 4, file);
    assert(header == EXPECTED_HEADER);

    struct world* world = malloc(sizeof(struct world));

    inventory_init(&world->inventory);
    camera_init(&world->camera, 70.0f, 0.5f, 30.0f);
    player_init(&world->player, &world->camera.transform, &world->inventory);
    camera_controller_init(&world->camera_controller, &world->camera, &world->player);

    pause_menu_init(&world->pause_menu, &world->inventory);

    uint16_t static_count;
    fread(&static_count, 2, 1, file);

    world->static_entities = malloc(sizeof(struct static_entity) * static_count);
    world->static_entity_count = static_count;

    for (int i = 0; i < static_count; ++i) {
        uint8_t str_len;
        fread(&str_len, 1, 1, file);

        struct static_entity* entity = &world->static_entities[i];

        entity->flags = 0;

        if (str_len == 0) {
            struct mesh* result = malloc(sizeof(struct mesh));
            mesh_load(result, file);
            entity->mesh = result;
            entity->flags |= STATIC_ENTITY_FLAGS_EMBEDDED_MESH;
        } else {
            char mesh_filename[str_len + 1];
            fread(mesh_filename, str_len, 1, file);
            mesh_filename[str_len] = '\0';
            entity->mesh = mesh_cache_load(mesh_filename);
        }
    }

    mesh_collider_load(&world->mesh_collider, file);
    collision_scene_use_static_collision(&world->mesh_collider);

    fread(&world->entity_data_count, 2, 1, file);

    world->entity_data = malloc(sizeof(struct entity_data) * world->entity_data_count);

    for (int i = 0; i < world->entity_data_count; i += 1) {
        world_load_entity(world, &world->entity_data[i], file);
    }

    fclose(file);

    render_scene_add(&r_scene_3d, NULL, 0.0f, world_render, world);

    return world;
}

void world_release(struct world* world) {
    for (int i = 0; i < world->static_entity_count; ++i) {
        struct static_entity* entity = &world->static_entities[i];
        if (entity->flags & STATIC_ENTITY_FLAGS_EMBEDDED_MESH) {
            mesh_release(entity->mesh);
            free(entity->mesh);
        } else {
            mesh_cache_release(entity->mesh);
        }
    }

    render_scene_remove(&r_scene_3d, world);

    pause_menu_destroy(&world->pause_menu);
    player_destroy(&world->player);
    camera_controller_destroy(&world->camera_controller);

    inventory_destroy(&world->inventory);

    collision_scene_remove_static_collision(&world->mesh_collider);
    mesh_collider_release(&world->mesh_collider);

    free(world);
}