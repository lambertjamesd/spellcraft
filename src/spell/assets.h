#ifndef __SPELL_ASSETS_H__
#define __SPELL_ASSETS_H__

#include "../render/tmesh.h"
#include "../render/animation_clip.h"

struct spell_assets {
    struct tmesh* projectile_mesh;
    struct tmesh* projectile_ice_mesh;
    struct tmesh* projectile_appear;
    struct animation_clip* projectile_appear_clip;
    struct tmesh* sheild_mesh;
    
    material_pair_t* fire_particle_mesh;
    material_pair_t* ice_particle_mesh;
    material_pair_t* dash_trail_material;
    material_pair_t* sword_trail_material;

    material_pair_t* ice_material;
    material_pair_t* shock_material;

    struct tmesh* fire_around_mesh;
    struct tmesh* fire_sweep_mesh;
    struct tmesh* fire_push_mesh;

    struct tmesh* ice_around_mesh;
    struct tmesh* ice_sweep_mesh;
    struct tmesh* ice_push_mesh;

    struct tmesh* flame_mesh;
    struct tmesh* lightning_around_mesh;
    struct tmesh* lightning_mesh;
    struct tmesh* lightning_strike;
    struct tmesh* lightning_cloud;
    struct tmesh* lightning_impact;
    material_pair_t* thunder_cloud;
    
    material_pair_t* spell_symbols;

    struct tmesh* fire_sprite;
    
    struct tmesh* water_around_mesh;
    struct tmesh* water_spray_mesh;

    struct tmesh* heal_aoe_mesh;
    struct tmesh* life_sprite;

    struct tmesh* fire_sword;
    struct tmesh* ice_sword;
};

void spell_assets_init();
struct spell_assets* spell_assets_get();

#endif