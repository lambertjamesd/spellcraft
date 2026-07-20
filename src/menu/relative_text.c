#include "relative_text.h"

// gross
#include "../../libdragon/src/rdpq/rdpq_font_internal.h"

/** @brief Hint for the compiler that the condition is likely to happen */
#define LIKELY(cond)  	__builtin_expect(!!(cond), 1)
/** @brief Hint for the compiler that the condition is unlikely to happen */
#define UNLIKELY(cond)  __builtin_expect(!!(cond), 0)

static void apply_style(int font_type, style_t *s, tex_format_t fmt)
{
    switch (font_type) {
    case FONT_TYPE_MONO_OUTLINE:
    case FONT_TYPE_ALIASED_OUTLINE:
        rdpq_set_env_color(s->outline_color);
        // fallthrough
    case FONT_TYPE_ALIASED:
    case FONT_TYPE_MONO:
    case FONT_TYPE_BITMAP:
        rdpq_set_prim_color(s->color);
        break;
    default:
        assert(0);
    }
    //Blender setup
    switch (font_type) {
        case FONT_TYPE_MONO:
            if(s->color.a != 255) {
                rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
            } else {
                rdpq_mode_blender(0);
            }
            break;

        case FONT_TYPE_MONO_OUTLINE:
        case FONT_TYPE_ALIASED_OUTLINE:
        case FONT_TYPE_ALIASED:
            rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
            break;
        
        case FONT_TYPE_BITMAP:
            if(s->color.a != 255 || fmt == FMT_RGBA32) {
                rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
            } else {
                rdpq_mode_blender(0);
            }
            break;
    }
    if (s->custom)
        s->custom(s->custom_arg);
}

int rsp_menu_font_render_paragraph(const rdpq_font_t *fnt, const rdpq_paragraph_char_t *chars, float x0, float y0, int relative_vtx)
{
    uint8_t font_id = chars[0].font_id;
    int cur_atlas = -1;
    int cur_style = -1;
    int rdram_loading = 0;
    int tile_offset = 0;

    const rdpq_paragraph_char_t *ch = chars;
    while (ch->font_id == font_id) {
        bool force_apply_style = false;
        const glyph_t *g = &fnt->glyphs[ch->glyph];
        if (UNLIKELY(g->natlas != cur_atlas)) {
            atlas_t *a = &fnt->atlases[g->natlas];
            rspq_block_run(a->up);
            if (a->sprite->hslices == 0) { // check if the atlas is in RDRAM instead of TMEM
                tile_offset = 0;
                switch (fnt->flags & FONT_FLAG_TYPE_MASK) {
                case FONT_TYPE_MONO:            rdram_loading = 1; break;
                case FONT_TYPE_MONO_OUTLINE:    rdram_loading = 1; break;
                case FONT_TYPE_ALIASED:         rdram_loading = 1; break;
                case FONT_TYPE_ALIASED_OUTLINE: rdram_loading = 2; break;
                case FONT_TYPE_BITMAP: switch (TEX_FORMAT_BITDEPTH(sprite_get_format(a->sprite))) {
                    case 4:     rdram_loading = 1; break;
                    default:    rdram_loading = 2; break;
                    } break;
                default: assert(0);
                }
            } else {
                rdram_loading = 0;
            }
            cur_atlas = g->natlas;
            force_apply_style = true;
        }
        if (force_apply_style || UNLIKELY(ch->style_id != cur_style)) {
            assertf(ch->style_id < fnt->num_styles,
                 "style %d not defined in this font", ch->style_id);
            apply_style(fnt->flags & FONT_FLAG_TYPE_MASK, &fnt->styles[ch->style_id], sprite_get_format(fnt->atlases[g->natlas].sprite));
            cur_style = ch->style_id;
        }

        // Draw the glyph
        float x = x0 + (ch->x + g->xoff);
        float y = y0 + (ch->y + g->yoff);
        int width = g->xoff2 - g->xoff;
        int height = g->yoff2 - g->yoff;
        int ntile = g->ntile;

        // Check if the atlas is in RDRAM (rather than TMEM). If so, we need
        // to load each glyph into TMEM before drawing.
        if (UNLIKELY(rdram_loading)) {
            switch (rdram_loading) {
            case 1: // 4bpp format: TILE4 is for loading, TILE0-3 are for rendering
                // If the atlas is 4bpp, we need to load the glyph as CI8 (usual trick)
                // TILE4 is the CI8 tile configured for loading
                rdpq_load_tile(TILE4, g->s/2, g->t, (g->s+width+1)/2, g->t+height);
                rdpq_set_tile_size(ntile+tile_offset, g->s & ~1, g->t, (g->s+width+1) & ~1, g->t+height);
                break;
            case 2: // 8bpp: all tiles can be used for both loading and rendering. ntile is always 0
                rdpq_load_tile(tile_offset, g->s, g->t, g->s+width, g->t+height);
                ntile = tile_offset;
                tile_offset = (tile_offset + 1) & 7;
                break;
            default:
                assertf(0, "invalid rdram_loading value %d", rdram_loading);
            }
        }

        menu_relative_tex_rect(
            relative_vtx,
            ntile,
            (int16_t)(x*4.0f), (int16_t)(y*4.0), (int16_t)((x+width) * 4.0f), (int16_t)((y+height) * 4.0f),
            (uint16_t)g->s << 5, (uint16_t)g->t<< 5,
            1 << 10, 1 << 10
        );

        ch++;
    }

    return ch - chars;
}

void rsp_menu_render_paragraph(const rdpq_paragraph_t *layout, float x0, float y0, int relative_vtx) {
    const rdpq_paragraph_char_t *ch = layout->chars;

    if (layout->flags & RDPQ_PARAGRAPH_FLAG_ANTIALIAS_FIX) {
        rdpq_mode_begin();
            rdpq_set_mode_standard();
            rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
            rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,0),(0,0,0,0)));
        rdpq_mode_end();

        // Draw a rectangle that covers three horizontal pixels on horizontal edges,
        // and one pixel on vertical edges. This makes sure the VI AA filter will
        // never fetch one of the text pixels.
        rdpq_fill_rectangle(layout->bbox.x0 + x0 - 3, layout->bbox.y0 + y0 - 1, layout->bbox.x1 + x0 + 6, layout->bbox.y1 + y0 + 2);
    }

    x0 += layout->x0;
    y0 += layout->y0;
    while (ch->font_id != 0) {
        const rdpq_font_t *fnt = rdpq_text_get_font(ch->font_id);
        int n = rsp_menu_font_render_paragraph(fnt, ch, x0, y0, relative_vtx);
        ch += n;
        assert(ch <= layout->chars + layout->nchars);
    }
}