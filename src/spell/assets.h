#ifndef __SPELL_ASSETS_H__
#define __SPELL_ASSETS_H__

#include "../render/tmesh.h"
#include "../render/animation_clip.h"

struct spell_assets {
    struct tmesh* projectile_mesh;
    struct tmesh* projectile_appear;
    struct animation_clip* projectile_appear_clip;
    struct tmesh* sheild_mesh;
    
    struct material* fire_particle_mesh;
    struct material* ice_particle_mesh;
    struct material* dash_trail_material;

    struct material* ice_material;
    struct material* shock_material;

    struct tmesh* fire_around_mesh;
    struct tmesh* fire_sweep_mesh;
    struct tmesh* fire_push_mesh;

    struct tmesh* ice_around_mesh;
    struct tmesh* ice_sweep_mesh;
    struct tmesh* ice_push_mesh;

    struct tmesh* flame_mesh;
    struct tmesh* lightning_around_mesh;
    struct tmesh* lightning_mesh;
    
    struct material* spell_symbols;
    struct material* casting_border;

    struct tmesh* fire_sprite;
    
    struct tmesh* water_around_mesh;
    struct tmesh* water_spray_mesh;
};

void spell_assets_init();
struct spell_assets* spell_assets_get();

#endif