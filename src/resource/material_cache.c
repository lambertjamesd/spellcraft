#include "material_cache.h"

#include <libdragon.h>
#include <malloc.h>
#include "resource_cache.h"

// MATR
#define EXPECTED_HEADER 0x4D415452

#define COMMAND_EOF         0x00
#define COMMAND_COMBINE     0x01
#define COMMAND_ENV         0x02
#define COMMAND_LIGHTING    0x03

void material_load(struct material* into, const char* path) {
    FILE* materialFile = asset_fopen(path, NULL);

    int header;
    fread(&header, 1, 4, materialFile);
    assert(header == EXPECTED_HEADER);

    bool hasMore = true;

    material_init(into);

    glNewList(into->list, GL_COMPILE);

    while (hasMore) {
        uint8_t nextCommand;
        fread(&nextCommand, 1, 1, materialFile);

        switch (nextCommand) {
            case COMMAND_EOF:
                hasMore = false;
                break;
            case COMMAND_COMBINE:
                {
                    rdpq_combiner_t combineMode;
                    fread(&combineMode, sizeof(rdpq_combiner_t), 1, materialFile);
                    rdpq_mode_combiner(combineMode);
                }
                break;
            case COMMAND_ENV:
                {
                    color_t color;
                    fread(&color, sizeof(color_t), 1, materialFile);
                    rdpq_set_env_color(color);
                }
                break;
            case COMMAND_LIGHTING:
                {
                    uint8_t enabled;
                    fread(&enabled, 1, 1, materialFile);
                    if (enabled) {
                        glEnable(GL_LIGHTING);
                    } else {
                        glDisable(GL_LIGHTING);
                    }
                }
                break;
        }
    }

    glEndList();

    fclose(materialFile);
}

struct resource_cache material_resource_cache;

struct material* material_cache_load(const char* filename) {
    struct resource_cache_entry* entry = resource_cache_use(&material_resource_cache, filename);

    if (!entry->resource) {
        struct material* result = malloc(sizeof(struct material));
        
        material_init(result);
        material_load(result, filename);

        entry->resource = result;
    }

    return entry->resource;
}

void material_cache_release(struct material* material) {
    if (resource_cache_free(&material_resource_cache, material)) {
        material_free(material);
        free(material);
    }
}