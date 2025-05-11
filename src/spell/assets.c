#include "assets.h"

#include "../resource/tmesh_cache.h"
#include "../resource/material_cache.h"
#include "../resource/animation_cache.h"

static struct spell_assets assets;

void spell_assets_init() {
    // tmesh_cache_release() never called
    assets.projectile_mesh = tmesh_cache_load("rom:/meshes/spell/rock.tmesh");
    // tmesh_cache_release() never called
    assets.projectile_appear = tmesh_cache_load("rom:/meshes/spell/rock_spawn.tmesh");
    // tmesh_cache_release() never called
    assets.sheild_mesh = tmesh_cache_load("rom:/meshes/spell/rock_shield.tmesh");

    // animation_cache_release() never called
    struct animation_set* projectile_animations = animation_cache_load("rom:/meshes/spell/rock_spawn.anim");
    assets.projectile_appear_clip = animation_set_find_clip(projectile_animations, "appear");

    // material_cache_release() never called
    assets.fire_particle_mesh = material_cache_load("rom:/materials/spell/fire_particle.mat");
    // material_cache_release() never called
    assets.ice_particle_mesh = material_cache_load("rom:/materials/spell/ice_particle.mat");
    // material_cache_release() never called
    assets.dash_trail_material = material_cache_load("rom:/materials/spell/dash_trail.mat");

    // material_cache_release() never called
    assets.ice_material = material_cache_load("rom:/materials/objects/ice.mat");

    // material_cache_release() never called
    assets.shock_material = material_cache_load("rom:/materials/objects/shock.mat");

    // tmesh_cache_release() never called
    assets.fire_around_mesh = tmesh_cache_load("rom:/meshes/spell/fire_sphere.tmesh");
    // tmesh_cache_release() never called
    assets.fire_sweep_mesh = tmesh_cache_load("rom:/meshes/spell/flame_sweep.tmesh");
    // tmesh_cache_release() never called
    assets.fire_push_mesh = tmesh_cache_load("rom:/meshes/spell/fire.tmesh");

    // tmesh_cache_release() never called
    assets.ice_around_mesh = tmesh_cache_load("rom:/meshes/spell/ice_sphere.tmesh");
    // tmesh_cache_release() never called
    assets.ice_sweep_mesh = tmesh_cache_load("rom:/meshes/spell/ice_sweep.tmesh");
    // tmesh_cache_release() never called
    assets.ice_push_mesh = tmesh_cache_load("rom:/meshes/spell/ice_push.tmesh");

    // tmesh_cache_release() never called
    assets.flame_mesh = tmesh_cache_load("rom:/meshes/objects/torch_flame.tmesh");

    // tmesh_cache_release() never called
    assets.lightning_around_mesh = tmesh_cache_load("rom:/meshes/spell/lightning_ball.tmesh");
    // tmesh_cache_release() never called
    assets.lightning_mesh = tmesh_cache_load("rom:/meshes/spell/lightning.tmesh");

    // material_cache_release() never called
    assets.spell_symbols = material_cache_load("rom:/materials/spell/symbols.mat");

    // material_cache_release() never called
    assets.casting_border = material_cache_load("rom:/materials/spell/casting_border.mat");

    // tmesh_cache_release() never called
    assets.fire_sprite = tmesh_cache_load("rom:/meshes/spell/fire_sprite.tmesh");

    // tmesh_cache_release() never called
    assets.water_around_mesh = tmesh_cache_load("rom:/meshes/spell/water_sphere.tmesh");

    // tmesh_cache_release() never called
    assets.water_spray_mesh = tmesh_cache_load("rom:/meshes/spell/water_spray.tmesh");

    // tmesh_cache_release() never called
    assets.heal_aoe_mesh = tmesh_cache_load("rom:/meshes/spell/heal_aeo.tmesh");
    // tmesh_cache_release() never called
    assets.life_sprite = tmesh_cache_load("rom:/meshes/spell/life_sprite.tmesh");
}

struct spell_assets* spell_assets_get() {
    return &assets;
}