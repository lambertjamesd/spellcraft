#ifndef __RENDER_TMESH_H__
#define __RENDER_TMESH_H__

#include <libdragon.h>
#include <t3d/t3d.h>

struct tmesh {
    rspq_block_t* block;
    T3DVertPacked* vertices;
    uint16_t vertex_count;
};

void tmesh_load(struct tmesh* tmesh, FILE* file);
void tmesh_release(struct tmesh* tmesh);

#endif