#ifndef __RENDER_TMESH_H__
#define __RENDER_TMESH_H__

#include <libdragon.h>
#include <t3d/t3d.h>

#include "material.h"
#include "armature.h"

struct tmesh {
    struct material* material;
    struct material* transition_materials;
    rspq_block_t* block;
    T3DVertPacked* vertices;
    uint16_t vertex_count;
    uint16_t material_transition_count;
    struct armature_definition armature;
    // this is a single instance for the entire mesh this should be instanced in the future after t3d supports it
    T3DMat4FP *armature_pose;
};

void tmesh_load(struct tmesh* tmesh, FILE* file);
void tmesh_release(struct tmesh* tmesh);

#endif