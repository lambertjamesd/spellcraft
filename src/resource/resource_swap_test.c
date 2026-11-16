#include "../test/framework_test.h"

#include <string.h>
#include <malloc.h>

#include "../render/tmesh.h"
#include "../render/animation_clip.h"
#include "../render/material.h"

void sprite_cache_swap(void* a, void* b);
void tmesh_cache_swap(void* a, void* b);
void material_cache_swap(void* a, void* b);
void animation_cache_swap(void* a, void* b);

/////////////////////////
// sprite
/////////////////////////

static sprite_t* test_sprite_new(int width, int height, tex_format_t format, uint16_t fill) {
    int stride = TEX_FORMAT_PIX2BYTES(format, width);
    sprite_t* result = malloc(sizeof(sprite_t) + stride * height);
    memset(result, 0, sizeof(sprite_t));
    result->width = width;
    result->height = height;
    result->flags = format;
    result->hslices = 1;
    result->vslices = 1;

    uint16_t* pixels = (uint16_t*)result->data;
    for (int i = 0; i < stride * height / 2; i += 1) {
        pixels[i] = fill;
    }

    return result;
}

static bool test_sprite_filled_with(sprite_t* sprite, uint16_t fill) {
    surface_t surface = sprite_get_pixels(sprite);
    uint16_t* pixels = surface.buffer;

    for (int i = 0; i < surface.stride * surface.height / 2; i += 1) {
        if (pixels[i] != fill) {
            return false;
        }
    }

    return true;
}

static void test_sprite_swap(struct test_context* t) {
    sprite_t* existing = test_sprite_new(8, 4, FMT_RGBA16, 0x1111);
    sprite_t* next = test_sprite_new(8, 4, FMT_RGBA16, 0x2222);

    sprite_cache_swap(next, existing);
    // the pixels are copied into the existing sprite
    test_eqi(t, true, test_sprite_filled_with(existing, 0x2222));
    test_eqi(t, 8, existing->width);
    test_eqi(t, FMT_RGBA16, sprite_get_format(existing));

    free(existing);
    free(next);
}

static void test_sprite_swap_width_changed(struct test_context* t) {
    sprite_t* existing = test_sprite_new(8, 4, FMT_RGBA16, 0x1111);
    sprite_t* next = test_sprite_new(16, 2, FMT_RGBA16, 0x2222);

    // same size in bytes but a different width should still be rejected
    sprite_cache_swap(next, existing);
    test_eqi(t, true, test_sprite_filled_with(existing, 0x1111));
    test_eqi(t, 8, existing->width);
    test_eqi(t, true, test_sprite_filled_with(next, 0x2222));

    free(existing);
    free(next);
}

static void test_sprite_swap_format_changed(struct test_context* t) {
    sprite_t* existing = test_sprite_new(8, 4, FMT_RGBA16, 0x1111);
    sprite_t* next = test_sprite_new(8, 4, FMT_IA16, 0x2222);

    sprite_cache_swap(next, existing);
    test_eqi(t, true, test_sprite_filled_with(existing, 0x1111));
    test_eqi(t, FMT_RGBA16, sprite_get_format(existing));

    free(existing);
    free(next);
}

static void test_sprite_swap_height_changed(struct test_context* t) {
    sprite_t* existing = test_sprite_new(8, 4, FMT_RGBA16, 0x1111);
    sprite_t* next = test_sprite_new(8, 8, FMT_RGBA16, 0x2222);

    // copying a taller sprite would overflow the existing allocation
    sprite_cache_swap(next, existing);
    test_eqi(t, true, test_sprite_filled_with(existing, 0x1111));
    test_eqi(t, 4, existing->height);

    free(existing);
    free(next);
}

/////////////////////////
// tmesh
/////////////////////////

static char* test_strdup(const char* str) {
    char* result = malloc(strlen(str) + 1);
    strcpy(result, str);
    return result;
}

static void test_tmesh_init(tmesh_t* mesh, int bone_count, int marker, const char** attachment_names, int attachment_count) {
    memset(mesh, 0, sizeof(tmesh_t));
    mesh->block = (rspq_block_t*)(marker * 16);
    mesh->vertices = (T3DVertPacked*)(marker * 16 + 4);
    mesh->vertex_count = marker;
    mesh->radius = marker;
    mesh->armature.bone_count = bone_count;
    mesh->armature.default_pose = (struct armature_packed_transform*)(marker * 16 + 8);

    mesh->attachment_count = attachment_count;
    mesh->attachments = attachment_count ? malloc(sizeof(armature_attachment_t) * attachment_count) : NULL;

    for (int i = 0; i < attachment_count; i += 1) {
        mesh->attachments[i].name = test_strdup(attachment_names[i]);
        mesh->attachments[i].bone_index = marker * 10 + i;
        mesh->attachments[i].local_pos = (vector3_t){marker, i, 0};
    }
}

static void test_tmesh_free(tmesh_t* mesh) {
    for (int i = 0; i < mesh->attachment_count; i += 1) {
        free(mesh->attachments[i].name);
    }
    free(mesh->attachments);
}

static void test_tmesh_swap(struct test_context* t) {
    const char* existing_names[] = {"hand", "removed"};
    const char* next_names[] = {"added", "hand"};

    tmesh_t existing;
    tmesh_t next;
    test_tmesh_init(&existing, 3, 1, existing_names, 2);
    test_tmesh_init(&next, 3, 2, next_names, 2);

    armature_attachment_t* existing_attachments = existing.attachments;
    armature_attachment_t* hand = &existing.attachments[0];
    armature_attachment_t* next_attachments = next.attachments;

    tmesh_cache_swap(&next, &existing);

    // the mesh data is swapped so the old data gets released with next
    test_eqi(t, 2 * 16, (int)existing.block);
    test_eqi(t, 2 * 16 + 4, (int)existing.vertices);
    test_eqi(t, 2 * 16 + 8, (int)existing.armature.default_pose);
    test_eqi(t, 2, existing.vertex_count);
    test_eqi(t, 1 * 16, (int)next.block);
    test_eqi(t, 1 * 16 + 4, (int)next.vertices);
    test_eqi(t, 1 * 16 + 8, (int)next.armature.default_pose);

    // attachments are held by pointer so they are updated in place
    test_eqi(t, (int)existing_attachments, (int)existing.attachments);
    test_eqi(t, (int)next_attachments, (int)next.attachments);
    test_eqi(t, 2, existing.attachment_count);
    test_str_equal(t, "hand", hand->name);
    test_eqi(t, 21, hand->bone_index);
    test_near_equalf(t, 2.0f, hand->local_pos.x);
    test_near_equalf(t, 1.0f, hand->local_pos.y);

    // a removed attachment keeps its old values
    test_str_equal(t, "removed", existing.attachments[1].name);
    test_eqi(t, 11, existing.attachments[1].bone_index);

    test_tmesh_free(&existing);
    test_tmesh_free(&next);
}

static void test_tmesh_swap_bone_count_changed(struct test_context* t) {
    const char* names[] = {"hand"};

    tmesh_t existing;
    tmesh_t next;
    test_tmesh_init(&existing, 3, 1, names, 1);
    test_tmesh_init(&next, 4, 2, names, 1);

    tmesh_cache_swap(&next, &existing);

    // nothing should change
    test_eqi(t, 1 * 16, (int)existing.block);
    test_eqi(t, 3, existing.armature.bone_count);
    test_eqi(t, 10, existing.attachments[0].bone_index);
    test_eqi(t, 2 * 16, (int)next.block);
    test_eqi(t, 4, next.armature.bone_count);

    test_tmesh_free(&existing);
    test_tmesh_free(&next);
}

static void test_tmesh_swap_placeholder_count_changed(struct test_context* t) {
    tmesh_t existing;
    tmesh_t next;
    test_tmesh_init(&existing, 3, 1, NULL, 0);
    test_tmesh_init(&next, 3, 2, NULL, 0);
    existing.armature.placeholder_count = 1;
    next.armature.placeholder_count = 2;

    tmesh_cache_swap(&next, &existing);

    test_eqi(t, 1 * 16, (int)existing.block);
    test_eqi(t, 1, existing.armature.placeholder_count);
    test_eqi(t, 2 * 16, (int)next.block);

    test_tmesh_free(&existing);
    test_tmesh_free(&next);
}

/////////////////////////
// material
/////////////////////////

static void test_material_swap(struct test_context* t) {
    material_pair_t existing;
    material_pair_t next;
    memset(&existing, 0, sizeof(existing));
    memset(&next, 0, sizeof(next));

    existing.apply.block = (rspq_block_t*)16;
    existing.apply.sort_priority = 1;
    next.apply.block = (rspq_block_t*)32;
    next.apply.sort_priority = 2;

    material_cache_swap(&next, &existing);

    test_eqi(t, 32, (int)existing.apply.block);
    test_eqi(t, 2, existing.apply.sort_priority);
    // the old material is left in next so it can be released
    test_eqi(t, 16, (int)next.apply.block);
    test_eqi(t, 1, next.apply.sort_priority);
}

/////////////////////////
// animation
/////////////////////////

// builds an animation set the same way animation_set_load does
// with the names and attributes sharing a buffer
static animation_set_t* test_animation_set_new(int bone_count, const char** names, int clip_count, int marker) {
    animation_set_t* result = malloc(sizeof(animation_set_t));
    result->bone_count = bone_count;
    result->clip_count = clip_count;
    result->clips = malloc(sizeof(animation_clip_t) * clip_count);

    int name_length = 0;
    for (int i = 0; i < clip_count; i += 1) {
        name_length += strlen(names[i]) + 1;
    }

    char* text_buffer = malloc(name_length);
    struct animation_used_attributes* attributes = malloc(sizeof(struct animation_used_attributes) * bone_count * clip_count);

    for (int i = 0; i < clip_count; i += 1) {
        animation_clip_t* clip = &result->clips[i];
        memset(clip, 0, sizeof(animation_clip_t));
        strcpy(text_buffer, names[i]);
        clip->name = text_buffer;
        clip->bone_count = bone_count;
        clip->frame_count = marker * 10 + i;
        clip->frames_rom_address = marker * 1000 + i;
        clip->used_bone_attributes = attributes;

        for (int bone = 0; bone < bone_count; bone += 1) {
            attributes[bone].has_pos = marker & 1;
            attributes[bone].has_rot = (marker >> 1) & 1;
            attributes[bone].has_scale = 0;
        }

        text_buffer += strlen(names[i]) + 1;
        attributes += bone_count;
    }

    return result;
}

static void test_animation_swap(struct test_context* t) {
    const char* existing_names[] = {"idle", "removed", "walk"};
    const char* next_names[] = {"walk", "idle", "added"};

    animation_set_t* existing = test_animation_set_new(2, existing_names, 3, 1);
    animation_set_t* next = test_animation_set_new(2, next_names, 3, 2);

    animation_clip_t* idle = animation_set_find_clip(existing, "idle");
    char* idle_name = idle->name;

    animation_cache_swap(next, existing);

    // clips are updated in place and keep their own names
    test_eqi(t, (int)idle, (int)animation_set_find_clip(existing, "idle"));
    test_eqi(t, (int)idle_name, (int)idle->name);
    test_eqi(t, 21, idle->frame_count);
    test_eqi(t, 2001, idle->frames_rom_address);
    test_eqi(t, 0, idle->used_bone_attributes[0].has_pos);
    test_eqi(t, 1, idle->used_bone_attributes[1].has_rot);

    animation_clip_t* walk = animation_set_find_clip(existing, "walk");
    test_eqi(t, 20, walk->frame_count);

    // removed clips keep their old data
    animation_clip_t* removed = animation_set_find_clip(existing, "removed");
    test_eqi(t, 11, removed->frame_count);
    test_eqi(t, 1, removed->used_bone_attributes[0].has_pos);

    // both sets must still own their buffers. this would
    // corrupt the heap if the clips had been swapped
    annotation_clip_set_free(existing);
    annotation_clip_set_free(next);
}

static void test_animation_swap_bone_count_changed(struct test_context* t) {
    const char* names[] = {"idle"};

    animation_set_t* existing = test_animation_set_new(2, names, 1, 1);
    animation_set_t* next = test_animation_set_new(3, names, 1, 2);

    animation_cache_swap(next, existing);

    test_eqi(t, 10, existing->clips[0].frame_count);
    test_eqi(t, 2, existing->bone_count);

    annotation_clip_set_free(existing);
    annotation_clip_set_free(next);
}

void test_resource_swap(struct test_context* t) {
    test_sprite_swap(t);
    test_sprite_swap_width_changed(t);
    test_sprite_swap_format_changed(t);
    test_sprite_swap_height_changed(t);
    test_tmesh_swap(t);
    test_tmesh_swap_bone_count_changed(t);
    test_tmesh_swap_placeholder_count_changed(t);
    test_material_swap(t);
    test_animation_swap(t);
    test_animation_swap_bone_count_changed(t);
}
