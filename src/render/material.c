#include "material.h"

#include <t3d/t3d.h>
#include "../resource/sprite_cache.h"
#include "../render/defs.h"

void material_init(struct material* material) {
    material->block = 0;

    material->tex0.sprite = NULL;
    material->tex0.reuse_prev_texture = false;

    material->tex1.sprite = NULL;
    material->tex1.reuse_prev_texture = false;

    material->sort_priority = SORT_PRIORITY_OPAQUE;

    material->palette.tlut = 0;
    material->palette.idx = 0;
    material->palette.size = 0;
    material->flags = 0;
}

void material_destroy(struct material* material) {
    if (material->tex0.sprite) {
        sprite_cache_release(material->tex0.sprite);
    }
    if (material->tex1.sprite) {
        sprite_cache_release(material->tex1.sprite);
    }
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
#define COMMAND_UV_GEN      0x08
#define COMMAND_FOG         0x09
#define COMMAND_FOG_COLOR   0x0A
#define COMMAND_FOG_RANGE   0x0B

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

    fread(&tex->scroll_x, sizeof(float), 1, file);
    fread(&tex->scroll_y, sizeof(float), 1, file);

    if (strcmp(filename, "reuse") == 0) {
        tex->reuse_prev_texture = true;
    } else {
        tex->sprite = sprite_cache_load(filename);
    }

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
        rdpq_sprite_upload(TILE0, into->tex0.sprite, &into->tex0.params);
    }

    if (into->tex1.sprite) {
        rdpq_sprite_upload(TILE1, into->tex1.sprite, &into->tex1.params);
    } else if (into->tex1.reuse_prev_texture) {
        int pitch_shift = into->tex0.sprite->format == FMT_RGBA32 ? 1 : 0;
        rdpq_tileparms_t tile_params;
        tile_params.palette = 1;
        tile_params.s.clamp = false;
        tile_params.s.mirror = false;
        tile_params.s.mask = 6;
        tile_params.s.shift = -1;
        
        tile_params.t.clamp = false;
        tile_params.t.mirror = false;
        tile_params.t.mask = 6;
        tile_params.t.shift = -1;
        rdpq_set_tile(
            TILE1, 
            into->tex0.sprite->format, 
            into->tex0.params.tmem_addr, 
            ((TEX_FORMAT_PIX2BYTES(into->tex0.sprite->format, into->tex0.sprite->width) >> pitch_shift) + 0x7) & ~0x7, 
            &tile_params
        );

        // s0 = s0*4 + tload->rect.s0fx;
        // t0 = t0*4 + tload->rect.t0fx;
        // s1 = s1*4 + tload->rect.s1fx;
        // t1 = t1*4 + tload->rect.t1fx;
        rdpq_set_tile_size_fx(TILE1, 0, 0, 64 << 2, 64 << 2);
    }

    if (autoLayoutTMem) {
        rdpq_tex_multi_end();
    }

    rdpq_mode_begin();

    rdpq_mode_filter(FILTER_BILINEAR);

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
                    rdpq_mode_blender(blendMode & SOM_BLEND_MASK);  

                    if (blendMode & SOM_Z_COMPARE) {
                        into->flags |= MATERIAL_FLAGS_Z_READ;
                    }                  

                    if (blendMode & SOM_Z_WRITE) {
                        into->flags |= MATERIAL_FLAGS_Z_WRITE;
                    }

                    if ((blendMode & SOM_ALPHACOMPARE_MASK) != 0) {
                        if ((blendMode & SOM_ALPHACOMPARE_MASK) == SOM_ALPHACOMPARE_THRESHOLD) {
                            rdpq_mode_alphacompare(128);
                        } else {
                            rdpq_mode_alphacompare(-1);
                        }
                    } else {
                        rdpq_mode_alphacompare(0);
                    }

                    // TODO check when the zmode is decal
                    // into->sort_priority = SORT_PRIORITY_DECAL;

                    if ((blendMode & SOM_Z_WRITE) == 0) {
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
                    fread(into->palette.tlut, sizeof(uint16_t), into->palette.size, material_file);
                    rdpq_tex_upload_tlut(into->palette.tlut, into->palette.idx, into->palette.size);
                }
                break;
            case COMMAND_UV_GEN:
                {
                    uint8_t fn;
                    fread(&fn, 1, 1, material_file);

                    switch (fn) {
                        case T3D_VERTEX_FX_NONE:
                            t3d_state_set_vertex_fx(T3D_VERTEX_FX_NONE, 0, 0);
                            break;
                        case T3D_VERTEX_FX_SPHERICAL_UV:
                            t3d_state_set_vertex_fx(T3D_VERTEX_FX_SPHERICAL_UV, into->tex0.sprite->width, into->tex0.sprite->height);
                            break;
                    }
                }
                break;
            case COMMAND_FOG:
                {
                    uint8_t enabled;
                    fread(&enabled, 1, 1, material_file);
                    t3d_fog_set_enabled(enabled);
                }
                break;
            case COMMAND_FOG_COLOR:
                {   
                    color_t color;
                    fread(&color, sizeof(color_t), 1, material_file);
                    rdpq_set_fog_color(color);
                }
                break;
            case COMMAND_FOG_RANGE:
                {
                    uint16_t min;
                    uint16_t max;
                    fread(&min, sizeof(min), 1, material_file);
                    fread(&max, sizeof(max), 1, material_file);
                    t3d_fog_set_range(min * (1.0f / SCENE_SCALE), max * (1.0f / SCENE_SCALE));
                }
                break;
        }
    }

    rdpq_mode_end();

    into->block = rspq_block_end();
}

void material_release(struct material* material) {
    material_destroy(material);
}