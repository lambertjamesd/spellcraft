#include "pottery_wheel.h"

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../math/vector2.h"

#include "../cutscene/expression_evaluate.h"

#define NEXT_ORIENTATION(value) (((value) + 1) % MAX_WHEEL_ORIENTATIONS)

#define MAX_ANGLE   2.0f

static struct Vector2 orientations[MAX_WHEEL_ORIENTATIONS] = {
    {1.0f, 0.0f},
    {0.0f, 1.0f},
    {-1.0f, 0.0f},
    {0.0f, -1.0f},
};

void pottery_wheel_update(void* data) {
    pottery_wheel_t* wheel = (pottery_wheel_t*)data;

    bool input_value = expression_get_bool(wheel->input);

    bool has_arrived = vector2RotateTowards(
        &wheel->transform.rotation,
        &orientations[wheel->target_orientation],
        &wheel->max_rotation,
        &wheel->transform.rotation
    );

    if (has_arrived && wheel->target_orientation) {
        expression_set_integer(wheel->output, wheel->target_orientation);
    }

    if (input_value && !wheel->last_input_value && has_arrived) {
        wheel->target_orientation = NEXT_ORIENTATION(wheel->target_orientation);
    }

    wheel->last_input_value = input_value;
}

void pottery_wheel_init(pottery_wheel_t* wheel, struct pottery_wheel_definition* definition, entity_id entity_id) {
    transformSaInit(&wheel->transform, &definition->position, &gRight2, 1.0f);
    renderable_single_axis_init(&wheel->renderable, &wheel->transform, "rom:/meshes/puzzle/pottery_wheel.tmesh");
    render_scene_add_renderable(&wheel->renderable, 3.0f);

    update_add(wheel, pottery_wheel_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);

    wheel->target_orientation = 0;
    wheel->last_input_value = 0;
    wheel->input = definition->input;
    wheel->output = definition->output;

    vector2ComplexFromAngle(MAX_ANGLE * fixed_time_step, &wheel->max_rotation);
}

void pottery_wheel_destroy(pottery_wheel_t* wheel) {
    render_scene_remove(&wheel->renderable);
    renderable_destroy(&wheel->renderable);
    update_remove(wheel);
}

void pottery_wheel_common_init() {

}

void pottery_wheel_common_destroy() {

}