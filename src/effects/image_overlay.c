#include "image_overlay.h"

#include <libdragon.h>
#include "../menu/menu_rendering.h"
#include "../render/material.h"

struct image_overlay {
    sprite_t* overlay;
    material_t material;
};

static struct image_overlay image_overlay;

void image_overlay_render(void* data) {
    material_apply(&image_overlay.material);
    rdpq_sprite_blit(image_overlay.overlay, 0, 0, NULL);
}

void image_overlay_set(const char* sprite_filename) {
    if ((image_overlay.overlay == NULL) != (sprite_filename == NULL)) {
        if (sprite_filename) {
            menu_add_callback(image_overlay_render, &image_overlay, MENU_PRIORITY_MENU);
            material_load_file(&image_overlay.material, "rom:/materials/menu/sprite_opaque.mat");
        } else {
            menu_remove_callback(&image_overlay);
            material_destroy(&image_overlay.material);
        }
    }

    if (image_overlay.overlay) {
        sprite_free(image_overlay.overlay);
        image_overlay.overlay = NULL;
    }

    if (sprite_filename) {
        image_overlay.overlay = sprite_load(sprite_filename);
    }
}