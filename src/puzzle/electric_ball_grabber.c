#include "electric_ball_grabber.h"

#include "../collision/collision_scene.h"
#include "../cutscene/expression_evaluate.h"
#include "../render/render_scene.h"
#include "../time/time.h"
#include "electric_ball.h"

#define CENTER_OFFSET       1.0f
#define GRAB_TIME_DELAY     0.5f

static color_t lit_color = {0xAE, 0xD8, 0x25, 0xFF};
static color_t unlit_color = {0x2A, 0x34, 0x09, 0xFF};

static spatial_trigger_type_t grabber_trigger_type = {
    SPATIAL_TRIGGER_SPHERE(1.0f),
    .center = {0.0f, CENTER_OFFSET, 0.0f},
};

void electric_ball_grabber_update(void* data) {
    electric_ball_grabber_t* grabber = (electric_ball_grabber_t*)data;

    contact_t* contact = grabber->trigger.active_contacts;

    bool is_grabbing = false;

    if (contact) {
        dynamic_object_t* obj = collision_scene_find_object(contact->other_object);

        if (obj) {
            struct Vector3 offset;
            vector3Sub(&grabber->transform.position, obj->position, &offset);
            offset.y += CENTER_OFFSET;
            vector3AddScaled(&obj->velocity, &offset, 1.0f, &obj->velocity);
            vector3Scale(&obj->velocity, &obj->velocity, 0.9f);
            is_grabbing = true;
        }
    }

    if (is_grabbing) {
        if (grabber->has_ball_time < GRAB_TIME_DELAY) {
            grabber->has_ball_time += fixed_time_step;

            if (grabber->has_ball_time >= GRAB_TIME_DELAY) {
                expression_set_bool(grabber->output, true);
                grabber->attrs[0].color = lit_color;
            }
        }
    } else {
        if (grabber->has_ball_time >= GRAB_TIME_DELAY) {
            expression_set_bool(grabber->output, false);
            grabber->attrs[0].color = unlit_color;
        } 
        
        if (grabber->has_ball_time > 0.0f) {            
            grabber->has_ball_time -= fixed_time_step;
        }
    }
}

void electric_ball_grabber_init(electric_ball_grabber_t* grabber, struct electric_ball_grabber_definition* definition, entity_id entity_id) {
    transformSaInit(&grabber->transform, &definition->position, &definition->rotation, 1.0f);

    spatial_trigger_init(&grabber->trigger, &grabber->transform, &grabber_trigger_type, COLLISION_LAYER_LIGHTNING_BALL);
    collision_scene_add_trigger(&grabber->trigger);

    grabber->attrs[0] = (element_attr_t){.type = ELEMENT_ATTR_PRIM_COLOR, .color = unlit_color};
    grabber->attrs[1].type = ELEMENT_ATTR_NONE;

    renderable_single_axis_init(&grabber->renderable, &grabber->transform, "rom:/meshes/puzzle/electric_ball_grabber.tmesh");
    grabber->renderable.attrs = grabber->attrs;

    render_scene_add_renderable(&grabber->renderable, 1.0f);

    update_add(grabber, electric_ball_grabber_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);

    grabber->has_ball_time = 0.0f;
    grabber->output = definition->output;
    grabber->entity_id = entity_id;

    struct Vector3 ball_hover_pos = definition->position;
    ball_hover_pos.y += CENTER_OFFSET;

    if (expression_get_bool(grabber->output)) {
        electric_ball_request_ball(&ball_hover_pos, entity_id, true);
    }
}

void electric_ball_grabber_destroy(electric_ball_grabber_t* grabber) {
    collision_scene_remove_trigger(&grabber->trigger);
    renderable_destroy(&grabber->renderable);
    render_scene_remove(&grabber->renderable);
    update_remove(grabber);
    electric_ball_remove_request(grabber->entity_id);
}