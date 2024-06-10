#include "material.h"

#include <t3d/t3d.h>
#include "../resource/sprite_cache.h"

void material_init(struct material* material) {
    material->block = 0;

    material->tex0.sprite = NULL;

    material->tex1.sprite = NULL;

    material->sort_priority = SORT_PRIORITY_OPAQUE;

    material->palette.tlut = 0;
    material->palette.idx = 0;
    material->palette.size = 0;
}

void material_free(struct material* material) {
    rspq_block_free(material->block);
    free(material->palette.tlut);
    material->palette.tlut = 0;
}

// MATR
#define EXPECTED_HEADER 0x4D415452

#define COMMAND_EOF         0x00
#define COMMAND_COMBINE     0x01
#define COMMAND_BLEND       0x02
#define COMMAND_ENV         0x03
#define COMMAND_PRIM        0x04
#define COMMAND_BLEND_COLOR 0x05
#define COMMAND_FLAGS       0x06
#define COMMAND_PALETTE     0x07

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

void material_load_tex(struct material_tex* tex, FILE* file, bool create_texture) {
    uint8_t filename_len;
    fread(&filename_len, 1, 1, file);

    if (filename_len == 0) {
        return;
    }

    char filename[filename_len + 1];
    fread(filename, 1, filename_len, file);
    filename[filename_len] = '\0';

    uint16_t tmem_addr;
    fread(&tmem_addr, 2, 1, file);
    tex->params.tmem_addr = tmem_addr;
    uint8_t palette;
    fread(&palette, 1, 1, file);
    tex->params.palette = palette;

    material_load_tex_axis((struct text_axis*)&tex->params.s, file);
    material_load_tex_axis((struct text_axis*)&tex->params.t, file);

    tex->sprite = sprite_cache_load(filename);

    uint8_t mag_filter;
    fread(&mag_filter, 1, 1, file);
    uint8_t min_filter;
    fread(&min_filter, 1, 1, file);

    if (!create_texture) {
        return;
    }
}

void material_load(struct material* into, FILE* material_file) {
    int header;
    fread(&header, 1, 4, material_file);
    assert(header == EXPECTED_HEADER);

    bool has_more = true;

    material_init(into);

    material_load_tex(&into->tex0, material_file, true);
    material_load_tex(&into->tex1, material_file, true);

    rspq_block_begin();

    bool autoLayoutTMem = into->tex1.sprite != 0 && into->tex1.params.tmem_addr == 0;

    if (autoLayoutTMem) {
        rdpq_tex_multi_begin();
    }

    if (into->tex0.sprite) {
        rdpq_mode_mipmap(MIPMAP_NONE, 0);
        surface_t surface = sprite_get_pixels(into->tex0.sprite);
        rdpq_tex_upload(TILE0, &surface, &into->tex0.params);

        // TODO remove this after implementing material revert
        rdpq_tlut_t tlut = rdpq_tlut_from_format(into->tex0.sprite->format);
        rdpq_mode_tlut(tlut);
    }

    if (into->tex1.sprite) {
        surface_t surface = sprite_get_pixels(into->tex1.sprite);
        rdpq_tex_upload(TILE1, &surface, &into->tex1.params);
    }

    if (autoLayoutTMem) {
        rdpq_tex_multi_end();
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
            case COMMAND_BLEND:
                {
                    rdpq_blender_t blendMode;
                    fread(&blendMode, sizeof(rdpq_blender_t), 1, material_file);
                    rdpq_mode_blender(blendMode);
                    // rdpq_change_other_modes_raw(SOM_ZMODE_MASK | SOM_Z_COMPARE | SOM_Z_WRITE | SOM_ALPHACOMPARE_MASK, blendMode);

                    if ((blendMode & SOM_ALPHACOMPARE_MASK) != 0) {
                        into->sort_priority = SORT_PRIORITY_DECAL;
                    }

                    if ((blendMode & SOM_Z_WRITE) != 0) {
                        into->sort_priority = SORT_PRIORITY_TRANSPARENT;
                    }
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
            case COMMAND_BLEND_COLOR:
                {
                    color_t color;
                    fread(&color, sizeof(color_t), 1, material_file);
                    rdpq_set_blend_color(color);
                }
                break;
            case COMMAND_FLAGS:
                {
                    uint16_t flags;
                    fread(&flags, 2, 1, material_file);
                    t3d_state_set_drawflags(flags);
                }
                break;
            case COMMAND_PALETTE:
                {
                    fread(&into->palette.idx, 2, 1, material_file);
                    fread(&into->palette.size, 2, 1, material_file);
                    into->palette.tlut = malloc(sizeof(uint16_t) * into->palette.size);
                    rdpq_tex_upload_tlut(into->palette.tlut, into->palette.idx, into->palette.size);
                }
                break;
        }
    }

    into->block = rspq_block_end();
}

void material_release(struct material* material) {
    material_free(material);
}