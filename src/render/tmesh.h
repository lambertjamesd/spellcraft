#ifndef __RENDER_TMESH_H__
#define __RENDER_TMESH_H__

#include <libdragon.h>
#include <t3d/t3d.h>

#include "material.h"
#include "armature.h"
#include "../math/transform.h"
#include "../math/vector3s16.h"
#include "../resource/incremental_loader.h"

enum light_source {
    LIGHT_SOURCE_NONE,
    LIGHT_SOURCE_CAMERA,
    LIGHT_SOURCE_RIM,
};

struct armature_attachment {
    char* name;
    uint16_t bone_index;
    vector3_t local_pos;
    T3DMat4FP local_transform;
};

typedef struct armature_attachment armature_attachment_t;

struct tmesh {
    material_pair_t* material;
    struct material* transition_materials;
    rspq_block_t* block;
    T3DVertPacked* vertices;
    uint16_t vertex_count;
    uint16_t material_transition_count;
    struct armature_definition armature;

    struct armature_attachment* attachments;
    material_placeholder_block_t* placeholders;
    float radius;
    uint16_t attachment_count;
    uint8_t light_source;
    uint8_t placeholder_count;
};

typedef struct tmesh tmesh_t;

incremental_step_result_t tmesh_load_incremental(incremental_loader_t* loader, incremental_loader_step_t* step);
void tmesh_load(struct tmesh* tmesh, FILE* file);
void tmesh_load_filename(struct tmesh* tmesh, const char* filename);
void tmesh_release(struct tmesh* tmesh);

void tmesh_apply_sweep(struct tmesh* tmesh, float time);

armature_attachment_t* tmesh_find_attachment(tmesh_t* mesh, const char* name);

void tmesh_compute_bounding_box(tmesh_t* mesh, vector3s16_t* min, vector3s16_t* max);

#endif