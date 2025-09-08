#include "lightning_strike.h"

#include "../time/time.h"
#include "../collision/collision_scene.h"
#include "../entity/health.h"
#include "../math/transform_single_axis.h"
#include "assets.h"
#include <t3d/t3dmath.h>

#define STRIKE_DELAY 0.5f
#define STRIKE_RANGE 0.5f
#define STRIKE_LIFETIME 1.5f

static damage_info_t strike_damage = {
    .amount = 10.0f,
    .knockback_strength = 0.0f,
    .direction = {0.0f, 1.0f, 0.0f},
    .type = DAMAGE_TYPE_LIGHTING,
};

void lightning_strike_start(struct lightning_strike* strike, struct Vector3* position, float delay, entity_id target) {
    strike->position = *position;
    strike->timer = -delay;
    strike->target = target;
}

void lightning_strike_damage_target(struct lightning_strike* strike) {
    health_t* health = health_get(strike->target);
    
    if (!health) {
        return;
    }

    dynamic_object_t* obj = collision_scene_find_object(strike->target);
    
    if (obj && vector3DistSqrd(obj->position, &strike->position) > STRIKE_RANGE * STRIKE_RANGE) {
        // target moved out of range
        return;
    }

    health_damage(health, &strike_damage);
}

void lightning_strike_update(struct lightning_strike* strike) {
    float start_time = strike->timer;
    strike->timer += fixed_time_step;

    if (strike->timer >= STRIKE_DELAY && start_time < STRIKE_DELAY) {

    }
}

void lightning_strike_render(struct lightning_strike* strike, render_batch_t* batch) {
    if (strike->target < STRIKE_DELAY || strike->target > STRIKE_LIFETIME) {
        return;
    }

    transform_sa_t transform = {
        .position = strike->position,
        .rotation = gRight2,
        .scale = 1.0f,
    };

    T3DMat4FP* mtx = render_batch_transformfp_from_sa(batch, &transform);

    render_batch_add_tmesh(batch, spell_assets_get()->lightning_strike, mtx, NULL, NULL, NULL);
}

bool lightning_strike_is_active(struct lightning_strike* strike) {
    return strike->timer < STRIKE_LIFETIME;
}