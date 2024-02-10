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
#define COMMAND_PRIM        0x03
#define COMMAND_LIGHTING    0x04

struct text_axis {
    float translate;
    int scale_log;
    float repeats;
    bool mirror;
};

void material_load_tex_axis(struct text_axis* axis, FILE* file) {
    int16_t translate;
    fread(&translate, 2, 1, file);
    axis->translate = translate * (1.0f / 32.0f);

    int8_t scale_log;
    fread(&scale_log, 1, 1, file);
    axis->scale_log = scale_log;

    uint16_t repeats;
    fread(&repeats, 2, 1, file);
    axis->repeats = repeats & 0x7FFF;

    axis->mirror = (repeats & 0x8000) != 0;
}

static GLenum material_filter_modes[] = {
    GL_NEAREST,
    GL_LINEAR,
};

void material_load_tex(struct material_tex* tex, FILE* file) {
    uint8_t filename_len;
    fread(&filename_len, 1, 1, file);

    if (filename_len == 0) {
        return;
    }

    char filename[filename_len + 1];
    fread(filename, 1, filename_len, file);
    filename[filename_len] = '\0';

    rdpq_texparms_t texparams;

    uint16_t tmem_addr;
    fread(&tmem_addr, 2, 1, file);
    texparams.tmem_addr = tmem_addr;
    uint8_t palette;
    fread(&palette, 1, 1, file);
    texparams.palette = palette;

    material_load_tex_axis((struct text_axis*)&texparams.s, file);
    material_load_tex_axis((struct text_axis*)&texparams.t, file);

    tex->sprite = sprite_cache_load(filename);

    glGenTextures(1, &tex->gl_texture);

    glBindTexture(GL_TEXTURE_2D, tex->gl_texture);

    uint8_t mag_filter;
    fread(&mag_filter, 1, 1, file);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, material_filter_modes[mag_filter]);

    fread(&mag_filter, 1, 1, file);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, material_filter_modes[mag_filter]);

    glSpriteTextureN64(GL_TEXTURE_2D, tex->sprite, &texparams);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void material_load(struct material* into, FILE* material_file) {
    int header;
    fread(&header, 1, 4, material_file);
    assert(header == EXPECTED_HEADER);

    bool has_more = true;

    material_init(into);

    material_load_tex(&into->tex0, material_file);

    glNewList(into->list, GL_COMPILE);

    if (into->tex0.gl_texture) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, into->tex0.gl_texture);
        glEnable(GL_RDPQ_MATERIAL_N64);
    }

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
            case COMMAND_PRIM:
                {
                    color_t color;
                    fread(&color, sizeof(color_t), 1, material_file);
                    rdpq_set_prim_color(color);
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
        }
    }

    glEndList();
}

void material_release(struct material* material) {
    if (material->tex0.gl_texture) {
        glDeleteTextures(1, &material->tex0.gl_texture);
    }

    if (material->tex0.sprite) {
        sprite_cache_release(material->tex0.sprite);
    }

    material_free(material);
}

struct resource_cache material_resource_cache;

struct material* material_cache_load(const char* filename) {
    struct resource_cache_entry* entry = resource_cache_use(&material_resource_cache, filename);

    if (!entry->resource) {
        struct material* result = malloc(sizeof(struct material));
        
        FILE* material_file = asset_fopen(filename, NULL);
        material_load(result, material_file);
        fclose(material_file);

        entry->resource = result;
    }

    return entry->resource;
}

void material_cache_release(struct material* material) {
    if (resource_cache_free(&material_resource_cache, material)) {
        material_release(material);
        free(material);
    }
}