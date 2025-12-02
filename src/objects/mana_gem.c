#include "mana_gem.h"
#include <malloc.h>
#include <stdbool.h>
#include "assets.h"

#include "../collision/collision_scene.h"
#include "../math/mathf.h"
#include "../render/defs.h"
#include "../render/render_scene.h"
#include "../time/time.h"
#include "../spell/mana_pool.h"
#include "../scene/scene_definition.h"

#define RADIUS_SCALE        0.02f
#define MIN_RADIUS          0.1f
#define COLLECT_DISTANCE    0.5f

static struct spatial_trigger_type mana_gem_trigger_type = {
    .type = SPATIAL_TRIGGER_CYLINDER,
    .data = {
        .cylinder = {
            .radius = 4.0f,
            .half_height = 4.0f,
        },
    }
};

void mana_gem_render(void* data, struct render_batch* batch) {
    struct mana_gem* gem = (struct mana_gem*)data;

    T3DMat4FP* mtxfp = render_batch_transformfp_from_sa(batch, &gem->transform);

    if (!mtxfp) {
        return;
    }

    render_batch_add_tmesh(
        batch, 
        object_assets_get()->mana_gem_mesh,
        mtxfp,
        NULL,
        NULL, 
        NULL
    );
}

struct mana_gem* mana_gem_new(struct Vector3* position, float mana_amount) {
    if (mana_amount <= 0.0f) {
        return NULL;
    }

    struct mana_gem* result = malloc(sizeof(struct mana_gem));

    if (!result) {
        return NULL;
    }

    mana_gem_init(result, position, mana_amount);
    return result;
}

void mana_gem_free(struct mana_gem* gem) {
    mana_gem_destroy(gem);
    free(gem);
}

bool mana_gem_gravitate_towards(struct mana_gem* gem, struct Vector3* target) {
    struct Vector3 offset;
    vector3Sub(target, &gem->transform.position, &offset);
    float dist_sqrd = vector3MagSqrd(&offset);

    if (dist_sqrd < COLLECT_DISTANCE * COLLECT_DISTANCE) {
        mana_pool_add(mana_pool_get(ENTITY_ID_PLAYER), gem->mana_amount);
        mana_gem_free(gem);
        return true;
    }

    vector3AddScaled(&gem->velocity, &offset, fixed_time_step / dist_sqrd, &gem->velocity);

    return false;
}

void mana_gem_update(void* data) {
    struct mana_gem* gem = (struct mana_gem*)data;

    vector3AddScaled(&gem->transform.position, &gem->velocity, fixed_time_step, &gem->transform.position);

    struct contact* contact = gem->trigger.active_contacts;

    bool is_gravitating = false;

    for (struct contact* contact = gem->trigger.active_contacts;
        contact;
        contact = contact->next
    ) {
        if (contact->other_object != ENTITY_ID_PLAYER) {
            continue;
        }

        struct dynamic_object* player = collision_scene_find_object(contact->other_object);

        if (!player) {
            continue;
        }

        struct Vector3 move_to;
        vector3Add(player->position, &player->center, &move_to);
        if (mana_gem_gravitate_towards(gem, &move_to)) {
            return;
        }
        is_gravitating = true;
    }

    if (!is_gravitating) {
        vector3Scale(&gem->velocity, &gem->velocity, 0.9f);
    }
}

void mana_gem_init(struct mana_gem* gem, struct Vector3* position, float mana_amount) {
    gem->velocity = gZeroVec;
    float radius = maxf(sqrtf(mana_amount) * RADIUS_SCALE, MIN_RADIUS);
    gem->mana_amount = mana_amount;
    transformSaInit(&gem->transform, position, &gRight2, radius);
    render_scene_add(&gem->transform.position, radius, mana_gem_render, gem);

    spatial_trigger_init(&gem->trigger, &gem->transform, &mana_gem_trigger_type, COLLISION_LAYER_DAMAGE_PLAYER, entity_id_new());
    collision_scene_add_trigger(&gem->trigger);

    update_add(gem, mana_gem_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);
}

void mana_gem_destroy(struct mana_gem* gem) {
    render_scene_remove(gem);
    collision_scene_remove_trigger(&gem->trigger);
    update_remove(gem);
}