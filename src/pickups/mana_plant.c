#include "mana_plant.h"

#include "../collision/collision_scene.h"
#include "../objects/assets.h"
#include "../render/render_scene.h"
#include "../resource/tmesh_cache.h"
#include "../time/time.h"
#include "../spell/mana_pool.h"

#define GEM_SCALE           0.291593f
#define GEM_OFFSET          0.938087f
#define GEM_COLLIDER_SIZE   0.5f

#define RESPAWN_TIME    5.0f
#define GROW_TIME       0.5f

#define MANA_AMOUNT     100.0f

static struct spatial_trigger_type mana_plant_grab_trigger = {
    .type = SPATIAL_TRIGGER_CYLINDER,
    .data = {
        .cylinder = {
            .half_height = GEM_OFFSET + GEM_SCALE,
            .radius = GEM_COLLIDER_SIZE,
        },
    },
};

void mana_plant_render(void* data, struct render_batch* batch) {
    struct mana_plant* plant = (struct mana_plant*)data;

    T3DMat4FP* mtxfp = render_batch_transformfp_from_sa(batch, &plant->transform, 1.0f);

    if (!mtxfp) {
        return;
    }

    render_batch_add_tmesh(
        batch, 
        plant->mesh,
        mtxfp,
        1,
        NULL,
        NULL
    );

    if (plant->respawn_timer > GROW_TIME) {
        return;
    }

    float grow_scale = (GROW_TIME - plant->respawn_timer) * (GEM_SCALE / GROW_TIME);

    if (grow_scale > GEM_SCALE) {
        grow_scale = GEM_SCALE;
    }

    struct TransformSingleAxis gem_transform = plant->transform;
    gem_transform.position.y += GEM_OFFSET;
    T3DMat4FP* gem_mtx = render_batch_transformfp_from_sa(batch, &gem_transform, grow_scale);

    if (!gem_mtx) {
        return;
    }

    render_batch_add_tmesh(
        batch, 
        object_assets_get()->mana_gem_mesh,
        gem_mtx,
        1,
        NULL,
        NULL
    );
}

void mana_plant_update(void* data) {
    struct mana_plant* plant = (struct mana_plant*)data;

    if (plant->respawn_timer > 0.0f) {
        plant->respawn_timer -= fixed_time_step;
    } else {
        struct contact* contact = plant->trigger.active_contacts;

        while (contact) {
            if (contact->other_object == ENTITY_ID_PLAYER) {
                plant->respawn_timer = RESPAWN_TIME;
                mana_pool_add(mana_pool_get(ENTITY_ID_PLAYER), MANA_AMOUNT);
                break;
            }

            contact = contact->next;
        }
    }
}

void mana_plant_init(struct mana_plant* plant, struct mana_plant_definition* definition) {
    plant->transform.position = definition->position;
    plant->transform.rotation = definition->rotation;
    plant->respawn_timer = 0.0f;

    plant->mesh = tmesh_cache_load("rom:/meshes/pickups/mana_plant.tmesh");
    render_scene_add(&plant->transform.position, 1.73f, mana_plant_render, plant);
    update_add(plant, mana_plant_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);

    spatial_trigger_init(
        &plant->trigger,
        &plant->transform,
        &mana_plant_grab_trigger,
        COLLISION_LAYER_DAMAGE_PLAYER
    );

    collision_scene_add_trigger(&plant->trigger);
}

void mana_plant_destroy(struct mana_plant* plant) {
    render_scene_remove(plant);
    update_remove(plant);
    collision_scene_remove_trigger(&plant->trigger);
}