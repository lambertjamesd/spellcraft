#include "animation_clip.h"

#include <string.h>
#include <malloc.h>
#include <libdragon.h>

struct animation_set_header {
    uint16_t clip_count;
    uint16_t name_buffer_length;
    uint16_t bone_count;
};

struct animation_clip_header {
    uint16_t frame_count;
    uint16_t frames_per_second;
    uint16_t frame_size;
};


// dma_read_raw_async
// dfs_rom_addr

struct animation_set* animation_set_load(const char* filename) {
    struct animation_set* result = malloc(sizeof(struct animation_set*));

    uint32_t start_address = dfs_rom_addr(filename);
    uint32_t current_address = start_address;

    struct animation_set_header header;

    dma_read(&header, current_address, sizeof(struct animation_set_header));
    current_address += sizeof(struct animation_set_header);

    result->clip_count = header.clip_count;
    result->bone_count = header.bone_count;

    if (result->clip_count == 0) {
        result->clips = NULL;
        return result;
    }

    struct animation_clip_header clip_headers[header.clip_count];
    dma_read(clip_headers, current_address, sizeof(struct animation_clip_header) * header.clip_count);
    current_address += sizeof(struct animation_clip_header) * header.clip_count;

    result->clips = malloc(sizeof(struct animation_clip) * header.clip_count);

    int attibute_buffer_size = sizeof(struct animation_used_attributes) * header.bone_count * header.clip_count;
    struct animation_used_attributes* attributes_buffer = malloc(attibute_buffer_size);
    dma_read(attributes_buffer, current_address, attibute_buffer_size);
    current_address += attibute_buffer_size;
    
    char* text_buffer = malloc(header.name_buffer_length);
    dma_read(text_buffer, current_address, header.name_buffer_length);
    // align to next 2 byte boundary
    current_address = (current_address + 1) & ~1;

    for (int i = 0; i < header.clip_count; i += 1) {
        struct animation_clip* clip = &result->clips[i];

        clip->name = text_buffer;

        clip->bone_count = header.bone_count;
        clip->frame_count = clip_headers[i].frame_count;
        clip->frames_per_second = clip_headers[i].frames_per_second;
        clip->frame_size = clip_headers[i].frame_size;
        clip->frames_rom_address = current_address;

        clip->used_bone_attributes = attributes_buffer;

        // advance to next string
        text_buffer += strlen(text_buffer) + 1;
        attributes_buffer += header.bone_count;
        current_address += clip_headers[i].frame_size * clip_headers[i].frame_count;
    }

    return result;
}

void annotation_clip_set_free(struct animation_set* animation_set) {
    if (!animation_set) {
        return;
    }

    if (animation_set->clip_count > 0) {
        free(animation_set->clips[0].name);
        free(animation_set->clips[0].used_bone_attributes);
        free(animation_set->clips);
    }

    free(animation_set);
}

struct animation_clip* animation_set_find_clip(struct animation_set* set, const char* clip_name) {
    for (int i = 0; i < set->clip_count; i += 1) {
        if (strcmp(set->clips[i].name, clip_name) == 0) {
            return &set->clips[i];
        }
    }

    return NULL;
}