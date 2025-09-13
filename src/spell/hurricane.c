#include "hurricane.h"

#include "../collision/collision_scene.h"
#include "../render/render_scene.h"
#include "../time/time.h"

static spatial_trigger_type_t hurricate_trigger = {
    SPATIAL_TRIGGER_CYLINDER(2.0f, 1.0f),
};

static struct Vector2 hurricane_rotate_rate;

#define ROTATE_RATE 2.9f

#define SCALE_IN_TIME   0.5f
#define SPELL_LIFETIME  6.0f
#define FADE_OUT_TIME   4.0f

void hurricane_init(hurricane_t* hurricane, spell_data_source_t* source, spell_event_options_t event_options) {
    transformSaInit(&hurricane->transform, &source->position, &gRight2, 0.0f);

    renderable_single_axis_init(&hurricane->renderable, &hurricane->transform, "rom:/meshes/spell/hurricane.tmesh");
    render_scene_add_renderable(&hurricane->renderable, 2.0f);

    vector2ComplexFromAngle(ROTATE_RATE * fixed_time_step, &hurricane_rotate_rate);

    hurricane->timer = 0.0f;

    hurricane->renderable.attrs = hurricane->attrs;
    hurricane->attrs[0].type = ELEMENT_ATTR_PRIM_COLOR;
    hurricane->attrs[0].color = (color_t){255, 255, 255, 255};
    hurricane->attrs[1].type = ELEMENT_ATTR_NONE;
}

bool hurricane_update(hurricane_t* hurricane) {
    vector2ComplexMul(&hurricane->transform.rotation, &hurricane_rotate_rate, &hurricane->transform.rotation);

    hurricane->timer += fixed_time_step;

    if (hurricane->timer < SCALE_IN_TIME) {
        hurricane->transform.scale = hurricane->timer * (1.0f / SCALE_IN_TIME);
    } else {
        hurricane->transform.scale = 1.0f;
    }

    if (hurricane->timer > FADE_OUT_TIME) {
        hurricane->attrs[0].color.a = (uint8_t)(255.0f * (SPELL_LIFETIME - hurricane->timer) * (1.0f / (SPELL_LIFETIME - FADE_OUT_TIME)));
    }

    return hurricane->timer < SPELL_LIFETIME;
}

void hurricane_destroy(hurricane_t* hurricane) {
    render_scene_remove(&hurricane->renderable);
    renderable_destroy(&hurricane->renderable);
}