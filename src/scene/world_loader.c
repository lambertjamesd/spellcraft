#include "world_loader.h"

#include <malloc.h>
#include <libdragon.h>
#include "../resource/mesh_cache.h"

// WRLD
#define EXPECTED_HEADER 0x57524C44

struct world* world_load(const char* filename) {
    FILE* file = asset_fopen(filename, NULL);

    int header;
    fread(&header, 1, 4, file);
    assert(header == EXPECTED_HEADER);

    struct world* world = malloc(sizeof(struct world));

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

    fclose(file);

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

    free(world);
}