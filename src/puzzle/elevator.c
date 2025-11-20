#include "elevator.h"

#include "../render/render_scene.h"
#include "../collision/collision_scene.h"
#include "../collision/shapes/cylinder.h"
#include "../time/time.h"
#include "../cutscene/expression_evaluate.h"
#include "../physics/move_towards.h"

#define END_DELAY_TIME  3.0f
#define MOVE_SPEED      3.0f

static struct move_towards_parameters move_parameters = {
    .max_speed = 5.0f,
    .max_accel = 4.0f,
};

static struct dynamic_object_type elevator_collision_type = {
    CYLINDER_COLLIDER(1.5f, 0.2211935f),
    .friction = 0.9f,
    .bounce = 0.0f,
};

void elevator_update(void* data) {
    struct elevator* elevator = (struct elevator*)data;
    bool enabled = expression_get_bool(elevator->enabled);

    if (elevator->inv_enabled) {
        enabled = !enabled;
    }

    if (!enabled) {
        move_towards(
            &elevator->transform.position, 
            &elevator->speed, 
            &elevator->start_position, 
            &move_parameters
        );
        elevator->timer = 0.0f;
        return;
    }

    if (elevator->timer > 0.0f) {
        elevator->timer -= fixed_time_step;
        return;
    }

    struct Vector3 move_to = elevator->is_returning ? elevator->start_position : elevator->end_position;

    struct Vector3 before = elevator->transform.position;

    if (move_towards(
        &elevator->transform.position, 
        &elevator->speed, 
        &move_to, 
        &move_parameters)
    ) {
        elevator->timer = END_DELAY_TIME;
        elevator->is_returning = !elevator->is_returning;
    }

    vector3Sub(&elevator->transform.position, &before, &elevator->collision.velocity);
    vector3Scale(&elevator->collision.velocity, &elevator->collision.velocity, 1.0f / fixed_time_step);
}

void elevator_init(struct elevator* elevator, struct elevator_definition* definition, entity_id id) {
    transformSaInit(&elevator->transform, &definition->position, &definition->rotation, 1.0f);
    elevator->start_position = definition->position;
    elevator->end_position = definition->target;

    renderable_single_axis_init(&elevator->renderable, &elevator->transform, "rom:/meshes/puzzle/elevator.tmesh");

    render_scene_add_renderable(&elevator->renderable, 2.0f);

    dynamic_object_init(
        id, 
        &elevator->collision, 
        &elevator_collision_type,
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE,
        &elevator->transform.position,
        &elevator->transform.rotation
    );

    elevator->collision.is_fixed = true;
    elevator->collision.weight_class = WEIGHT_CLASS_SUPER_HEAVY;

    elevator->timer = 0.0f;
    elevator->enabled = definition->enabled;
    elevator->inv_enabled = definition->inv_enabled;
    elevator->is_returning = false;
    elevator->speed = 0.0f;

    collision_scene_add(&elevator->collision);

    update_add(elevator, elevator_update, UPDATE_PRIORITY_PHYICS, UPDATE_LAYER_WORLD);
}

void elevator_destroy(struct elevator* elevator) {
    collision_scene_remove(&elevator->collision);
    render_scene_remove(&elevator->renderable);
    renderable_destroy(&elevator->renderable);
    update_remove(elevator);
}

void elevator_common_init() {

}

void elevator_common_destroy() {

}