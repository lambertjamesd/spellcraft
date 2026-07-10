#ifndef __RENDER_MESH2D__
#define __RENDER_MESH2D__

#include <libdragon.h>
#include "material.h"

struct mesh2d {
    material_pair_t* material;
    material_t* transition_materials;
    uint16_t material_transition_count;
    
    rspq_block_t* block;
};

typedef struct mesh2d mesh2d_t;

void mesh2d_load(mesh2d_t* mesh, FILE* file);
void mesh2d_release(mesh2d_t* mesh);

#endif