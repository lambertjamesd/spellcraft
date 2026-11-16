#include "animation_cache.h"

#include "resource_cache.h"

#include <string.h>
#include <libdragon.h>

static struct resource_cache animation_resource_cache;

void animation_cache_swap(void* a, void* b) {
    animation_set_t* new_resource = (animation_set_t*)a;
    animation_set_t* existing_resource = (animation_set_t*)b;

    if (existing_resource->bone_count != new_resource->bone_count) {
        debugf("error: number of bones changed when reloading animations\n");
        return;
    }

    // clip names and used_bone_attributes point into buffers owned by
    // clips[0] of each set so the clips can't be swapped wholesale. Instead
    // the existing clips keep their own buffers and the data is copied into
    // them so any animation_clip_t* held by an animator stays valid
    for (int i = 0; i < existing_resource->clip_count; i += 1) {
        animation_clip_t* existing_clip = &existing_resource->clips[i];
        animation_clip_t* clip = animation_set_find_clip(new_resource, existing_clip->name);

        if (!clip) {
            debugf("error: an animation was removed %s\n", existing_clip->name);
            continue;
        }

        existing_clip->frame_count = clip->frame_count;
        existing_clip->frames_per_second = clip->frames_per_second;
        existing_clip->frame_size = clip->frame_size;
        existing_clip->has_events = clip->has_events;
        existing_clip->has_image_frames = clip->has_image_frames;
        existing_clip->has_prim_color = clip->has_prim_color;
        existing_clip->has_env_color = clip->has_env_color;
        existing_clip->placecholders_size = clip->placecholders_size;
        existing_clip->frames_rom_address = clip->frames_rom_address;
        memcpy(
            existing_clip->used_bone_attributes, 
            clip->used_bone_attributes, 
            sizeof(struct animation_used_attributes) * existing_resource->bone_count
        );
    }

    for (int i = 0; i < new_resource->clip_count; i += 1) {
        if (!animation_set_find_clip(existing_resource, new_resource->clips[i].name)) {
            debugf("error: an animation was added %s, it will be available once the animation is no longer in use\n", new_resource->clips[i].name);
        }
    }
}

void animation_cache_init() {
    resource_cache_register(
        &animation_resource_cache, 
        (resource_cache_load)animation_set_load,
        (resource_cache_free_callback)annotation_clip_set_free,
        animation_cache_swap,
        ".anim"
    );
}

struct animation_set* animation_cache_load(const char* filename) {
    return resource_cache_use(&animation_resource_cache, filename);
}

void animation_cache_release(struct animation_set* animations) {
    resource_cache_free(&animation_resource_cache, animations);
}

void animation_cache_destroy() {
    resource_cache_destroy(&animation_resource_cache);
}