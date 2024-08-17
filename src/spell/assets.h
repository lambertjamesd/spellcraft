#ifndef __SPELL_ASSETS_H__
#define __SPELL_ASSETS_H__

#include "../render/tmesh.h"

struct spell_assets {
    struct tmesh* projectile_mesh;
    struct material* fire_particle_mesh;
    struct material* ice_particle_mesh;
    struct material* dash_trail_material;

    struct tmesh* fire_around_mesh;

    struct tmesh* flame_mesh;
};

void spell_assets_init();
struct spell_assets* spell_assets_get();

#endif