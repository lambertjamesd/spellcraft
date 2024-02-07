#include "material_cache.h"

#include <libdragon.h>
#include <malloc.h>
#include "resource_cache.h"
#include "sprite_cache.h"

// MATR
#define EXPECTED_HEADER 0x4D415452

#define COMMAND_EOF         0x00
#define COMMAND_COMBINE     0x01
#define COMMAND_ENV         0x02
#define COMMAND_LIGHTING    0x03
#define COMMAND_TEX0        0x04

void material_load(struct material* into, const char* path) {
    FILE* material_file = asset_fopen(path, NULL);

    int header;
    fread(&header, 1, 4, material_file);
    assert(header == EXPECTED_HEADER);

    bool has_more = true;

    material_init(into);

    glNewList(into->list, GL_COMPILE);

    while (has_more) {
        uint8_t nextCommand;
        fread(&nextCommand, 1, 1, material_file);

        switch (nextCommand) {
            case COMMAND_EOF:
                has_more = false;
                break;
            case COMMAND_COMBINE:
                {
                    rdpq_combiner_t combineMode;
                    fread(&combineMode, sizeof(rdpq_combiner_t), 1, material_file);
                    rdpq_mode_combiner(combineMode);
                }
                break;
            case COMMAND_ENV:
                {
                    color_t color;
                    fread(&color, sizeof(color_t), 1, material_file);
                    rdpq_set_env_color(color);
                }
                break;
            case COMMAND_LIGHTING:
                {
                    uint8_t enabled;
                    fread(&enabled, 1, 1, material_file);
                    if (enabled) {
                        glEnable(GL_LIGHTING);
                    } else {
                        glDisable(GL_LIGHTING);
                    }
                }
                break;
            case COMMAND_TEX0:
                {
                    uint8_t filename_len;
                    fread(&filename_len, 1, 1, material_file);
                    char filename[filename_len + 1];
                    fread(filename, 1, filename_len, material_file);
                    filename[filename_len] = '\0';

                    into->tex0_sprite = sprite_cache_load(filename);
                }
                break;
        }
    }

    glEndList();

    fclose(material_file);
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
        if (material->tex0_sprite) {
            sprite_cache_release(material->tex0_sprite);
        }

        material_free(material);
        free(material);
    }
}