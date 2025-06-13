#include "mana_gem.h"
#include <malloc.h>
#include <stdbool.h>
#include "assets.h"

#include "../collision/collision_scene.h"
#include "../math/mathf.h"
#include "../render/defs.h"
#include "../render/render_scene.h"
#include "../time/time.h"

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

    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    mat4x4 mtx;
    transformSAToMatrix(&gem->transform, mtx, gem->radius);
    render_batch_relative_mtx(batch, mtx);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    render_batch_add_tmesh(
        batch, 
        object_assets_get()->mana_gem_mesh,
        mtxfp,
        1,
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
    gem->transform.position = *position;
    gem->transform.rotation = gRight2;
    gem->velocity = gZeroVec;
    gem->radius = maxf(sqrtf(mana_amount) * RADIUS_SCALE, MIN_RADIUS);
    render_scene_add(&gem->transform.position, gem->radius, mana_gem_render, gem);

    spatial_trigger_init(&gem->trigger, &gem->transform, &mana_gem_trigger_type);
    collision_scene_add_trigger(&gem->trigger);

    update_add(gem, mana_gem_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);
}

void mana_gem_destroy(struct mana_gem* gem) {
    render_scene_remove(gem);
    collision_scene_remove_trigger(&gem->trigger);
    update_remove(gem);
}