#include "world_loader.h"

#include <malloc.h>
#include <libdragon.h>
#include "../resource/mesh_cache.h"
#include "../resource/mesh_collider.h"
#include "../render/render_scene.h"

#include "../collision/collision_scene.h"

// WRLD
#define EXPECTED_HEADER 0x57524C44

struct world* world_load(const char* filename) {
    FILE* file = asset_fopen(filename, NULL);

    int header;
    fread(&header, 1, 4, file);
    assert(header == EXPECTED_HEADER);

    struct world* world = malloc(sizeof(struct world));

    camera_init(&world->camera, 70.0f, 0.5f, 30.0f);
    player_init(&world->player, &world->camera.transform);
    camera_controller_init(&world->camera_controller, &world->camera, &world->player);

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

    fclose(file);

    world->static_render_id = render_scene_add(&r_scene_3d, NULL, 0.0f, world_render, world);

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

    render_scene_remove(&r_scene_3d, world->static_render_id);

    player_destroy(&world->player);
    camera_controller_destroy(&world->camera_controller);

    collision_scene_remove_static_collision(&world->mesh_collider);
    mesh_collider_release(&world->mesh_collider);

    free(world);
}