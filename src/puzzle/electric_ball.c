#include "electric_ball.h"

#include "../render/render_scene.h"
#include "../collision/collision_scene.h"
#include "../collision/shapes/sphere.h"

static struct dynamic_object_type electric_ball_collision_type = {
    SPHERE_COLLIDER(0.5f),
    .friction = 0.9f,
    .bounce = 0.2f,
};

void electric_ball_init(electric_ball_t* ball, struct electric_ball_definition* definition, entity_id entity_id) {
    transformSaInit(&ball->transform, &definition->position, &gRight2, 1.0f);

    renderable_single_axis_init(&ball->renderable, &ball->transform, "rom:/meshes/puzzle/electric_ball.tmesh");
    render_scene_add_renderable(&ball->renderable, 0.5f);

    dynamic_object_init(
        entity_id,
        &ball->collision,
        &electric_ball_collision_type,
        COLLISION_LAYER_TANGIBLE,
        &ball->transform.position,
        NULL
    );

    collision_scene_add(&ball->collision);
}

void electric_ball_destroy(electric_ball_t* ball) {
    render_scene_remove(&ball->renderable);
    renderable_destroy(&ball->renderable);
    collision_scene_remove(&ball->collision);
}