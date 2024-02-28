#include "assets.h"

#include "../resource/mesh_cache.h"
#include "../resource/material_cache.h"

static struct spell_assets assets;

void spell_assets_init() {
    assets.projectile_mesh = mesh_cache_load("rom:/meshes/spell/projectile.mesh");
    assets.fire_particle_mesh = material_cache_load("rom:/materials/spell/fire_particle.mat");
}

struct spell_assets* spell_assets_get() {
    return &assets;
}