#include "lightning_strike.h"

#include "../time/time.h"
#include "../collision/collision_scene.h"
#include "../entity/health.h"
#include "../math/transform_single_axis.h"
#include "../effects/fade_effect.h"
#include "../scene/scene.h"
#include "../render/coloru8.h"
#include "assets.h"
#include <t3d/t3dmath.h>

#define STRIKE_DELAY            0.333f
#define BOLT_TIME               1.0f
#define BOLT_END                (STRIKE_DELAY + BOLT_TIME)
#define CLOUD_COLOR_FADE_OUT    0.5f
#define CLOUD_FADE_OUT          1.0f
#define IMPACT_FADE_OUT         1.333f
#define CLOUD_GONE_TIME         (STRIKE_DELAY + BOLT_TIME + CLOUD_FADE_OUT)
#define STRIKE_LIFETIME         (STRIKE_DELAY + BOLT_TIME + CLOUD_FADE_OUT + IMPACT_FADE_OUT)

#define SCROLL_AMOUNT   41.0f

static damage_info_t strike_damage = {
    .amount = 10.0f,
    .knockback_strength = 0.0f,
    .direction = {0.0f, 1.0f, 0.0f},
    .type = DAMAGE_TYPE_LIGHTING,
};

struct color_keyframe {
    color_t color;
    float time;
};

static struct color_keyframe cloud_keyframes[] = {
    {{ 0x0D, 0x16, 0x15, 0x00 }, 0.0f},
    {{ 0x14, 0x41, 0x4F, 0xFF }, STRIKE_DELAY},
    {{ 0x0D, 0x16, 0x15, 0xFF }, STRIKE_DELAY + CLOUD_COLOR_FADE_OUT},
    {{ 0x0D, 0x16, 0x15, 0x00 }, CLOUD_GONE_TIME},
};

static struct color_keyframe impact_keyframes[] = {
    {{ 0xFF, 0xFF, 0xFF, 0xFF }, STRIKE_DELAY},
    {{ 0x00, 0x59, 0x6F, 0xFF }, STRIKE_DELAY + CLOUD_COLOR_FADE_OUT},
    {{ 0x00, 0x00, 0x00, 0xFF }, STRIKE_DELAY + BOLT_TIME + CLOUD_FADE_OUT},
    {{ 0x00, 0x00, 0x00, 0x00 }, STRIKE_DELAY + BOLT_TIME + CLOUD_FADE_OUT + IMPACT_FADE_OUT},
};

color_t lightning_eval_color(struct color_keyframe* keyframes, int count, float time) {
    if (time < keyframes[0].time) {
        return keyframes[0].color;
    }

    struct color_keyframe* curr = &keyframes[1];

    for (int i = 1; i < count; i += 1) {
        if (time < curr->time) {
            struct color_keyframe* prev = curr - 1;
            float lerp = (time - prev->time) / (curr->time - prev->time);
            struct Coloru8 result = coloru8_lerp((struct Coloru8*)&prev->color, (struct Coloru8*)&curr->color, lerp);
            return (color_t){result.r, result.g, result.b, result.a};
        }
        
        ++curr;
    }

    return keyframes[count - 1].color;
}

#define LIGHTNING_EVAL_COLOR(keyframes, time)   lightning_eval_color(keyframes, sizeof(keyframes) / sizeof(*keyframes), time)

void lightning_strike_start(struct lightning_strike* strike, struct Vector3* position, bool is_grounded) {
    strike->position = *position;
    strike->timer = 0.0f;
    strike->is_grounded = is_grounded;
}

bool lightning_strike_update(struct lightning_strike* strike) {
    float start_time = strike->timer;
    strike->timer += fixed_time_step;

    bool result = strike->timer >= STRIKE_DELAY && start_time < STRIKE_DELAY;

    if (result) {
        fade_effect_flash((struct Coloru8){
            .r = 0xFF,
            .g = 0xFF,
            .b = 0xFF,
            .a = 0x40,
        });
        camera_shake(&current_scene->camera_controller, 0.1f);
    }

    return result;
}

void lightning_strike_render(struct lightning_strike* strike, render_batch_t* batch) {
    if (strike->timer < 0.0f || strike->timer > STRIKE_LIFETIME) {
        return;
    }

    transform_sa_t transform = {
        .position = strike->position,
        .rotation = batch->rotation_2d,
        .scale = 1.0f,
    };

    T3DMat4FP* mtx = render_batch_transformfp_from_sa(batch, &transform);

    if (!mtx) {
        return;
    }

    if (strike->timer < CLOUD_GONE_TIME) {
        element_attr_t cloud_attrs[2];
        cloud_attrs[0] = (element_attr_t){.type = ELEMENT_ATTR_PRIM_COLOR, .color = LIGHTNING_EVAL_COLOR(cloud_keyframes, strike->timer)};
        cloud_attrs[1].type = ELEMENT_ATTR_NONE;
        render_batch_add_tmesh(batch, spell_assets_get()->lightning_cloud, mtx, NULL, NULL, cloud_attrs);
    }

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

    if (!strike->is_grounded || strike->timer < STRIKE_DELAY) {
        return;
    }

    transform.rotation = gRight2;

    mtx = render_batch_transformfp_from_sa(batch, &transform);

    if (!mtx) {
        return;
    }

    element_attr_t impact_attrs[2];
    impact_attrs[0] = (element_attr_t){
        .type = ELEMENT_ATTR_PRIM_COLOR, 
        .color = LIGHTNING_EVAL_COLOR(impact_keyframes, strike->timer),
    };
    impact_attrs[1].type = ELEMENT_ATTR_NONE;
    render_batch_add_tmesh(batch, spell_assets_get()->lightning_impact, mtx, NULL, NULL, impact_attrs);
}

bool lightning_strike_is_active(struct lightning_strike* strike) {
    return strike->timer < STRIKE_LIFETIME;
}