#include "sprite_cache.h"

#include <string.h>
#include "resource_cache.h"
#include "../util/cleanup.h"

struct resource_cache sprite_resource_cache;

static int sprite_cache_palette_size(sprite_t* sprite) {
    if (!sprite_get_palette(sprite)) {
        return 0;
    }

    return sprite_get_palette_used_colors(sprite);
}

// materials and display lists hold onto the address of the pixel data
// so instead of swapping the sprite the pixel data is copied in place
void sprite_cache_swap(void* a, void* b) {
    sprite_t* new_resource = (sprite_t*)a;
    sprite_t* existing_resource = (sprite_t*)b;

    tex_format_t format = sprite_get_format(existing_resource);
    tex_format_t new_format = sprite_get_format(new_resource);

    if (existing_resource->width != new_resource->width || format != new_format) {
        debugf(
            "error: sprite width or format changed when reloading (%dx%d %s) -> (%dx%d %s)\n",
            existing_resource->width, existing_resource->height, tex_format_name(format),
            new_resource->width, new_resource->height, tex_format_name(new_format)
        );
        return;
    }

    // the pixel data is copied into the existing allocation so it has to be the same size
    if (existing_resource->height != new_resource->height) {
        debugf("error: sprite height changed when reloading %d -> %d\n", existing_resource->height, new_resource->height);
        return;
    }

    int lod_count = sprite_get_lod_count(existing_resource);

    if (lod_count != sprite_get_lod_count(new_resource)) {
        debugf("error: sprite mipmap count changed when reloading %d -> %d\n", lod_count, sprite_get_lod_count(new_resource));
        return;
    }

    int palette_size = sprite_cache_palette_size(new_resource);

    if (palette_size > sprite_cache_palette_size(existing_resource)) {
        debugf("error: sprite palette grew when reloading %d -> %d\n", sprite_cache_palette_size(existing_resource), palette_size);
        return;
    }

    // check every lod before copying anything so a rejected swap leaves the sprite untouched
    for (int i = 0; i < lod_count; i += 1) {
        surface_t existing_lod = sprite_get_lod_pixels(existing_resource, i);
        surface_t new_lod = sprite_get_lod_pixels(new_resource, i);

        if (existing_lod.stride * existing_lod.height != new_lod.stride * new_lod.height) {
            debugf("error: sprite mipmap %d changed size when reloading\n", i);
            return;
        }
    }

    for (int i = 0; i < lod_count; i += 1) {
        surface_t existing_lod = sprite_get_lod_pixels(existing_resource, i);
        surface_t new_lod = sprite_get_lod_pixels(new_resource, i);
        int size = existing_lod.stride * existing_lod.height;
        memcpy(existing_lod.buffer, new_lod.buffer, size);
        // the RDP reads the pixels straight from RDRAM
        data_cache_hit_writeback(existing_lod.buffer, size);
    }

    if (palette_size) {
        uint16_t* palette = sprite_get_palette(existing_resource);
        memcpy(palette, sprite_get_palette(new_resource), sizeof(uint16_t) * palette_size);
        data_cache_hit_writeback(palette, sizeof(uint16_t) * palette_size);
    }
}

void sprite_cache_init() {
    resource_cache_register(
        &sprite_resource_cache,
        (resource_cache_load)sprite_load,
        (resource_cache_free_callback)sprite_free,
        sprite_cache_swap,
        ".sprite"
    );
}

sprite_t* sprite_cache_load(const char* filename) {
    return resource_cache_use(&sprite_resource_cache, filename);
}

// TODO cleanup
void sprite_cache_release_direct(sprite_t* sprite) {
    resource_cache_free(&sprite_resource_cache, sprite);
}

void sprite_cache_release(sprite_t* sprite) {
    resource_cache_free(&sprite_resource_cache, sprite);
}

void sprite_cache_destroy() {
    resource_cache_destroy(&sprite_resource_cache);
}
