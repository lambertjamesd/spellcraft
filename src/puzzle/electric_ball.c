#include "electric_ball.h"

#include "../render/render_scene.h"
#include "../collision/collision_scene.h"
#include "../collision/shapes/sphere.h"

static color_t lit_color = {0xAE, 0xD8, 0x25, 0xFF};
static color_t unlit_color = {0x2A, 0x34, 0x09, 0xFF};

static struct dynamic_object_type electric_ball_collision_type = {
    SPHERE_COLLIDER(0.5f),
    .friction = 0.9f,
    .bounce = 0.2f,
};

float electric_ball_on_damage(void* data, struct damage_info* damage) {
    electric_ball_t* ball = (electric_ball_t*)data;

    if (damage->type & DAMAGE_TYPE_LIGHTING) {
        ball->collision.collision_layers |= COLLISION_LAYER_LIGHTNING_BALL;
        ball->attrs[0].color = lit_color;
    } else if (damage->type & DAMAGE_TYPE_WATER) {
        ball->collision.collision_layers &= ~COLLISION_LAYER_LIGHTNING_BALL;
        ball->attrs[0].color = unlit_color;
    }

    return 0.0f;
}

void electric_ball_init(electric_ball_t* ball, struct electric_ball_definition* definition, entity_id entity_id) {
    transformSaInit(&ball->transform, &definition->position, &gRight2, 1.0f);

    renderable_single_axis_init(&ball->renderable, &ball->transform, "rom:/meshes/puzzle/electric_ball.tmesh");
    render_scene_add_renderable(&ball->renderable, 0.5f);

    ball->attrs[0] = (element_attr_t){
        .type = ELEMENT_ATTR_PRIM_COLOR, 
        .color = definition->is_energized ? lit_color : unlit_color,
    };
    ball->attrs[1].type = ELEMENT_ATTR_NONE;
    ball->renderable.attrs = &ball->attrs[0];

    health_init(&ball->health, entity_id, 0.0f);
    health_set_callback(&ball->health, electric_ball_on_damage, ball);

    dynamic_object_init(
        entity_id,
        &ball->collision,
        &electric_ball_collision_type,
        definition->is_energized ? 
            COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY | COLLISION_LAYER_Z_TARGET | COLLISION_LAYER_LIGHTNING_BALL : 
            COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY | COLLISION_LAYER_Z_TARGET,
        &ball->transform.position,
        NULL
    );

    collision_scene_add(&ball->collision);
}

void electric_ball_destroy(electric_ball_t* ball) {
    render_scene_remove(&ball->renderable);
    renderable_destroy(&ball->renderable);
    collision_scene_remove(&ball->collision);
    health_destroy(&ball->health);
}