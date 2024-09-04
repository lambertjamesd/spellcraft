#include "assets.h"

#include "../resource/tmesh_cache.h"
#include "../resource/material_cache.h"

static struct spell_assets assets;

void spell_assets_init() {
    assets.projectile_mesh = tmesh_cache_load("rom:/meshes/spell/projectile.tmesh");
    // material_cache_release() never called
    assets.fire_particle_mesh = material_cache_load("rom:/materials/spell/fire_particle.mat");
    // material_cache_release() never called
    assets.ice_particle_mesh = material_cache_load("rom:/materials/spell/ice_particle.mat");
    // material_cache_release() never called
    assets.dash_trail_material = material_cache_load("rom:/materials/spell/dash_trail.mat");

    // material_cache_release() never called
    assets.ice_material = material_cache_load("rom:/materials/objects/ice.mat");

    assets.fire_around_mesh = tmesh_cache_load("rom:/meshes/spell/flame_around.tmesh");

    assets.flame_mesh = tmesh_cache_load("rom:/meshes/objects/torch_flame.tmesh");

    assets.lightning_mesh = tmesh_cache_load("rom:/meshes/spell/lightning.tmesh");
}

struct spell_assets* spell_assets_get() {
    return &assets;
}