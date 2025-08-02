#include "show_item.h"

#include "../time/time.h"
#include "../menu/menu_common.h"
#include "../math/mathf.h"
#include "../resource/material_cache.h"
#include "../player/inventory.h"

#define RUNE_SHOW_TIME              3.0f
#define RUNE_FADE_TIME              0.25f
#define RUNE_FLASH_TIME             2.0f

static const char* tablet_images[] = {
    "rom:/images/menu/tablets/tablet_level_1.sprite",
    "rom:/images/menu/tablets/tablet_level_2.sprite",
    "rom:/images/menu/tablets/tablet_level_3.sprite",
    "rom:/images/menu/tablets/tablet_level_4.sprite",
    "rom:/images/menu/tablets/tablet_level_5.sprite",
};

static const char* symbol_material = "rom:/materials/spell/symbols.mat";

static const char* item_icon_materials[] = {
    [SPELL_SYMBOL_FIRE] = "rom:/materials/spell/symbols.mat",
    [SPELL_SYMBOL_ICE] = "rom:/materials/spell/symbols.mat",
    [SPELL_SYMBOL_EARTH] = "rom:/materials/spell/symbols.mat",
    [SPELL_SYMBOL_AIR] = "rom:/materials/spell/symbols.mat",
    [SPELL_SYMBOL_LIFE] = "rom:/materials/spell/symbols.mat",

    [SPELL_SYMBOL_RECAST] = "rom:/materials/spell/symbols.mat",
    // SPELL_SYMBOL_PASS_DOWN,

    [ITEM_TYPE_STAFF_DEFAULT] = "rom:/materials/objects/icons/default_staff_icon.mat",
};

static char* item_get_message[] = {
    [SPELL_SYMBOL_FIRE] = "You found the fire rune!\n\nWith it you can summon fire or imbue fire into chained runes",
    [SPELL_SYMBOL_EARTH] = "You found the projectile rune!\n\nUse it to damage enemies from afar or even chain into other runes on impact",
    [ITEM_TYPE_STAFF_DEFAULT] = "You got your fathers old staff",
};

static const char image_offset_x[] = {
    [SPELL_SYMBOL_FIRE] = 0,
    [SPELL_SYMBOL_ICE] = 24,
    [SPELL_SYMBOL_EARTH] = 48,
    [SPELL_SYMBOL_AIR] = 96,
    [SPELL_SYMBOL_LIFE] = 120,

    [SPELL_SYMBOL_RECAST] = 144,
    // SPELL_SYMBOL_PASS_DOWN,

    [ITEM_TYPE_STAFF_DEFAULT] = 0,
};

static const char image_width_x[] = {
    [SPELL_SYMBOL_FIRE] = 24,
    [SPELL_SYMBOL_ICE] = 24,
    [SPELL_SYMBOL_EARTH] = 24,
    [SPELL_SYMBOL_AIR] = 24,
    [SPELL_SYMBOL_LIFE] = 24,

    [SPELL_SYMBOL_RECAST] = 24,
    // SPELL_SYMBOL_PASS_DOWN,

    [ITEM_TYPE_STAFF_DEFAULT] = 64,
};

struct offset_rect {
    int8_t x, y;
    uint8_t w, h;
};

static const struct offset_rect level_offset_rects[] = {
    { 10, 11, 20, 20 },
    { 10, 11, 20, 20 },
    { 10, 13, 20, 20 },
    { 7, 7, 16, 16 },
    { 7, 7, 14, 14 },
};

bool show_item_is_spell(enum inventory_item_type type) {
    return type >= SPELL_SYMBOL_FIRE && type <= SPELL_SYMBOL_LIFE;
}

void show_item_init(struct show_item* show_item) {
    show_item->item_material = NULL;
    show_item->item_sprite = NULL;
    show_item->should_show = 0;
    show_item->show_item_timer = 0.0f;
    show_item->showing_item = 0;
}

void show_item_start(struct show_item* show_item, union cutscene_step_data* data) {
    show_item->should_show = data->show_item.should_show;

    if (data->show_item.should_show) {
        if (show_item_is_spell(data->show_item.item)) {
            int level = inventory_get_item_level(data->show_item.item) - 1;

            if (level < 0) {
                level = 0;
            } else if (level >= 5) {
                level = 4;
            }

            show_item->showing_item = data->show_item.item;
            show_item->show_item_timer = 0.0f;
            show_item->item_material = material_cache_load(symbol_material);
            show_item->item_sprite = sprite_load(tablet_images[level]);

        } else {
            show_item->showing_item = data->show_item.item;
            show_item->show_item_timer = 0.0f;
            show_item->item_material = material_cache_load(item_icon_materials[show_item->showing_item]);
            show_item->item_sprite = NULL;
        }
    } else {
        show_item->show_item_timer = RUNE_FADE_TIME;
    }
}

bool show_item_update(struct show_item* show_item, union cutscene_step_data* data) {
    float target = data->show_item.should_show ? RUNE_SHOW_TIME : 0.0f;
    show_item->show_item_timer = mathfMoveTowards(show_item->show_item_timer, target, fixed_time_step);

    joypad_buttons_t buttons = joypad_get_buttons_pressed(0);

    if (buttons.start) {
        show_item->show_item_timer = target;
    }

    bool result = show_item->show_item_timer == target;

    if (result && target == 0) {
        if (show_item->item_material) {
            material_cache_release(show_item->item_material);
        }
        show_item->item_material = NULL;
        if (show_item->item_sprite) {
            sprite_free(show_item->item_sprite);
        }
    }

    return result;
}

void show_item_render(struct show_item* show_item) {
    if (show_item->showing_item && show_item->show_item_timer) {
        rspq_block_run(solid_primitive_material->block);

        float alpha = show_item->show_item_timer * (1.0f / RUNE_FADE_TIME);

        if (alpha > 1.0f) {
            alpha = 1.0f;
        }

        color_t color = {0, 0, 0, (uint8_t)(alpha * 128.0f)};
        rdpq_set_prim_color(color);

        rdpq_texture_rectangle(TILE0, 0, 0, 320, 240, 0, 0);

        int size = (int)(32.0f * alpha);

        rspq_block_run(current_spell_icon->block);
        rdpq_texture_rectangle_scaled(
            TILE0, 
            160 - size, 80 - size,
            160 + size, 80 + size,
            0, 0,
            32, 32
        );

        bool is_showing = !show_item->showing_item || show_item->show_item_timer > RUNE_FADE_TIME;

        int x = 160 - 24;
        int y = 80 - 24;
        int w = 48;
        int h = 48;

        if (show_item->item_sprite && is_showing) {
            rspq_block_run(sprite_blit->block);
            rdpq_sprite_blit(show_item->item_sprite, 160 - 24, 80 - 24, NULL);

            int level = inventory_get_item_level(show_item->showing_item) - 1;

            if (level < 0) {
                level = 0;
            } else if (level >= 5) {
                level = 4;
            }

            const struct offset_rect* offsets = &level_offset_rects[level];

            x += offsets->x;
            y += offsets->y;
            w = offsets->w;
            h = offsets->h;
        }

        if (show_item->item_material && is_showing) {
            rspq_block_run(show_item->item_material->block);

            if (!show_item->showing_item) {
                color_t color = {255, 255, 255, (uint8_t)(alpha * 255.0f)};
                rdpq_set_prim_color(color);
            }

            int offset = image_offset_x[show_item->showing_item];
            int size = image_width_x[show_item->showing_item];

            rdpq_texture_rectangle_scaled(
                TILE0, 
                x, y,
                x + w, y + h,
                offset, 0,
                size + offset, show_item->item_material->tex0.sprite->height
            );
        }

        float flash_time = show_item->show_item_timer - RUNE_FADE_TIME;

        if (flash_time > 0.0f && flash_time < RUNE_FLASH_TIME) {
            float alpha = 1.0f - (flash_time * (1.0f / RUNE_FLASH_TIME));
            alpha *= alpha;

            rspq_block_run(solid_primitive_material->block);
            color_t color = {255, 255, 255, (uint8_t)(alpha * 255.0f)};
            rdpq_set_prim_color(color);
            rdpq_texture_rectangle(TILE0, 0, 0, 320, 240, 0, 0);
        }
    }
}

void show_item_in_cutscene(struct cutscene_builder* cutscene_builder, enum inventory_item_type item) {
    cutscene_builder_pause(cutscene_builder, true, true, UPDATE_LAYER_WORLD);
    cutscene_builder_show_item(cutscene_builder, item, true);
    
    if (item_get_message[item]) {
        cutscene_builder_dialog(cutscene_builder, item_get_message[item]);
    }

    cutscene_builder_show_item(cutscene_builder, 1, false);
    cutscene_builder_pause(cutscene_builder, false, true, UPDATE_LAYER_WORLD);
}