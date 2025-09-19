#include "lightning_strike.h"

#include "../time/time.h"
#include "../collision/collision_scene.h"
#include "../entity/health.h"
#include "../math/transform_single_axis.h"
#include "assets.h"
#include <t3d/t3dmath.h>

#define STRIKE_DELAY            0.333f
#define BOLT_TIME               1.0f
#define BOLT_END                (STRIKE_DELAY + BOLT_TIME)
#define CLOUD_COLOR_FADE_OUT    0.5f
#define CLOUD_FADE_OUT          1.0f
#define STRIKE_LIFETIME         (STRIKE_DELAY + BOLT_TIME + CLOUD_FADE_OUT)

#define SCROLL_AMOUNT   41.0f

static damage_info_t strike_damage = {
    .amount = 10.0f,
    .knockback_strength = 0.0f,
    .direction = {0.0f, 1.0f, 0.0f},
    .type = DAMAGE_TYPE_LIGHTING,
};

void lightning_strike_start(struct lightning_strike* strike, struct Vector3* position, bool is_grounded) {
    strike->position = *position;
    strike->timer = 0.0f;
    strike->is_grounded = is_grounded;
}

bool lightning_strike_update(struct lightning_strike* strike) {
    float start_time = strike->timer;
    strike->timer += fixed_time_step;

    return strike->timer >= STRIKE_DELAY && start_time < STRIKE_DELAY;
}

void lightning_strike_render(struct lightning_strike* strike, render_batch_t* batch) {
    if (strike->timer < 0.0f || strike->timer > STRIKE_LIFETIME) {
        return;
    }

    transform_sa_t transform = {
        .position = strike->position,
        .rotation = gRight2,
        .scale = 1.0f,
    };

    T3DMat4FP* mtx = render_batch_transformfp_from_sa(batch, &transform);

    if (!mtx) {
        return;
    }

    color_t cloud_color;

    if (strike->timer < STRIKE_DELAY) {
        cloud_color = (color_t){
            0x0D, 
            0x16, 
            0x15, 
            (uint8_t)(strike->timer * (255.0f / STRIKE_DELAY)),
        };
    } else if (strike->timer > BOLT_END) {
        cloud_color = (color_t) {
            0x0D, 
            0x16, 
            0x15, 
            (uint8_t)((STRIKE_LIFETIME - strike->timer) * (255.0f / CLOUD_FADE_OUT)),
        };
    } else if (strike->timer > STRIKE_DELAY + CLOUD_COLOR_FADE_OUT) {
        cloud_color = (color_t){
            0x0D, 
            0x16, 
            0x15, 
            0xFF,
        };
    } else {
        float lerp = (strike->timer - STRIKE_DELAY) * (1.0f / CLOUD_COLOR_FADE_OUT);
        cloud_color = (color_t){
            (int8_t)((0x0D - 0x14) * lerp) + 0x14, 
            (int8_t)((0x16 - 0x41) * lerp) + 0x41, 
            (int8_t)((0x15 - 0x4F) * lerp) + 0x4F, 
            0xFF,
        };
    }

    element_attr_t cloud_attrs[2];
    cloud_attrs[0] = (element_attr_t){.type = ELEMENT_ATTR_PRIM_COLOR, .color = cloud_color};
    cloud_attrs[1].type = ELEMENT_ATTR_NONE;
    render_batch_add_tmesh(batch, spell_assets_get()->lightning_cloud, mtx, NULL, NULL, cloud_attrs);

    float bolt_transparency = (BOLT_END - strike->timer) * (1.0f / BOLT_TIME);

    if (bolt_transparency >= 0.0f && bolt_transparency <= 1.0f) {
        element_attr_t attrs[3];
        attrs[0] = (element_attr_t){.type = ELEMENT_ATTR_PRIM_COLOR, .color = {0x00, 0x85, 0xFF, (uint8_t)(255.0f * bolt_transparency)}};
        attrs[1] = (element_attr_t){
            .type = ELEMENT_ATTR_SCROLL, 
            .scroll = {(int16_t)(SCROLL_AMOUNT * (1.0f - bolt_transparency)), 0},
        };
        attrs[2].type = ELEMENT_ATTR_NONE;
    
        render_batch_add_tmesh(batch, spell_assets_get()->lightning_strike, mtx, NULL, NULL, attrs);
    }

    if (strike->is_grounded) {
        
    }
}

bool lightning_strike_is_active(struct lightning_strike* strike) {
    return strike->timer < STRIKE_LIFETIME;
}

float lightning_strike_cloud_size(struct lightning_strike* strike) {
    if (strike->timer < 0.0f) {
        return 0.0f;
    }

    if (strike->timer < STRIKE_DELAY) {
        return strike->timer * (1.0f / STRIKE_DELAY);
    }

    if (strike->timer >= STRIKE_LIFETIME) {
        return 0.0f;
    }

    // return 1.0f;

    return (STRIKE_LIFETIME - strike->timer) * (1.0f / (STRIKE_LIFETIME - STRIKE_DELAY));
}