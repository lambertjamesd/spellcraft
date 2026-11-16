#include "tmesh_cache.h"

#include <string.h>
#include "resource_cache.h"
#include "../util/cleanup.h"

struct resource_cache tmesh_resource_cache;

void* tmesh_cache_entry_load(const char* filename) {
    struct tmesh* result = malloc(sizeof(struct tmesh));

    FILE* mesh_file = asset_fopen(filename, NULL);
    tmesh_load(result, mesh_file);
    fclose(mesh_file);

    return result;
}

void tmesh_cache_free_entry(void* mesh) {
    tmesh_release(mesh);
    free(mesh);
}

// armature_attachment_t pointers are held by other objects so the existing
// attachments array is kept and updated in place instead of being swapped
static void tmesh_cache_update_attachments(struct tmesh* new_resource, struct tmesh* existing_resource) {
    for (int i = 0; i < existing_resource->attachment_count; i += 1) {
        armature_attachment_t* existing_attachment = &existing_resource->attachments[i];
        armature_attachment_t* attachment = tmesh_find_attachment(new_resource, existing_attachment->name);

        if (!attachment) {
            debugf("error: an attachment was removed %s\n", existing_attachment->name);
            continue;
        }

        existing_attachment->bone_index = attachment->bone_index;
        existing_attachment->local_pos = attachment->local_pos;
        existing_attachment->local_transform = attachment->local_transform;
        data_cache_hit_writeback(&existing_attachment->local_transform, sizeof(T3DMat4FP));
    }

    for (int i = 0; i < new_resource->attachment_count; i += 1) {
        if (!tmesh_find_attachment(existing_resource, new_resource->attachments[i].name)) {
            debugf("error: an attachment was added %s, it will be available once the mesh is no longer in use\n", new_resource->attachments[i].name);
        }
    }
}

void tmesh_cache_swap(void* a, void* b) {
    struct tmesh* new_resource = (struct tmesh*)a;
    struct tmesh* existing_resource = (struct tmesh*)b;

    // armatures allocate their pose using the bone count of the definition
    if (existing_resource->armature.bone_count != new_resource->armature.bone_count) {
        debugf(
            "error: number of bones changed when reloading mesh %d -> %d\n",
            existing_resource->armature.bone_count,
            new_resource->armature.bone_count
        );
        return;
    }

    // armatures also allocate their placeholders using the placeholder count
    if (existing_resource->armature.placeholder_count != new_resource->armature.placeholder_count) {
        debugf(
            "error: number of material placeholders changed when reloading mesh %d -> %d\n",
            existing_resource->armature.placeholder_count,
            new_resource->armature.placeholder_count
        );
        return;
    }

    tmesh_cache_update_attachments(new_resource, existing_resource);

    // swap everything except the attachments. The old contents end
    // up in new_resource which is released after the swap
    armature_attachment_t* new_attachments = new_resource->attachments;
    uint16_t new_attachment_count = new_resource->attachment_count;
    armature_attachment_t* existing_attachments = existing_resource->attachments;
    uint16_t existing_attachment_count = existing_resource->attachment_count;

    struct tmesh tmp = *new_resource;
    *new_resource = *existing_resource;
    *existing_resource = tmp;

    existing_resource->attachments = existing_attachments;
    existing_resource->attachment_count = existing_attachment_count;
    new_resource->attachments = new_attachments;
    new_resource->attachment_count = new_attachment_count;
}

void tmesh_cache_init() {
    resource_cache_register(
        &tmesh_resource_cache,
        tmesh_cache_entry_load,
        tmesh_cache_free_entry,
        tmesh_cache_swap,
        ".tmesh"
    );
}

struct tmesh* tmesh_cache_load(const char* filename) {
    return resource_cache_use(&tmesh_resource_cache, filename);
}

// TODO cleanup
void tmesh_cache_release_direct(struct tmesh* mesh) {
    resource_cache_free(&tmesh_resource_cache, mesh);
}

void tmesh_cache_release(struct tmesh* mesh) {
    resource_cache_free(&tmesh_resource_cache, mesh);
}

void tmesh_cache_destroy() {
    resource_cache_destroy(&tmesh_resource_cache);
}
