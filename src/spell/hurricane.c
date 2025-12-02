#include "hurricane.h"

#include "../collision/collision_scene.h"
#include "../render/render_scene.h"
#include "../time/time.h"
#include "../entity/health.h"
#include "ground_movement.h"

static spatial_trigger_type_t hurricate_trigger = {
    SPATIAL_TRIGGER_CYLINDER(4.0f, 1.2f),
};

static struct Vector2 hurricane_rotate_rate;

static damage_info_t hurricane_damage = {
    .amount = 0.2f,
    .type = DAMAGE_TYPE_WATER,  
};

static move_over_ground_def_t move_def = {
    .move_speed = 2.0f,
    .max_upward_movment = 0.1f,
    .height_offset = 1.0f,
    .can_fly = true,
};

#define ROTATE_RATE 2.9f

#define SCALE_IN_TIME       0.5f
#define SPELL_LIFETIME      6.0f
#define FADE_OUT_TIME       4.0f
#define PUSH_FORCE          5.0f
#define TANGENT_FORCE_RATIO 0.1f

void hurricane_init(hurricane_t* hurricane, spell_data_source_t* source, spell_event_options_t event_options) {
    transformSaInit(&hurricane->transform, &source->position, &gRight2, 0.0f);

    renderable_single_axis_init(&hurricane->renderable, &hurricane->transform, "rom:/meshes/spell/hurricane.tmesh");
    render_scene_add_renderable(&hurricane->renderable, 2.0f);

    vector2ComplexFromAngle(ROTATE_RATE * fixed_time_step, &hurricane_rotate_rate);

    spatial_trigger_init(&hurricane->trigger, &hurricane->transform, &hurricate_trigger, COLLISION_LAYER_DAMAGE_ENEMY, entity_id_new());
    collision_scene_add_trigger(&hurricane->trigger);

    hurricane->timer = 0.0f;

    hurricane->renderable.attrs = hurricane->attrs;
    hurricane->attrs[0].type = ELEMENT_ATTR_PRIM_COLOR;
    hurricane->attrs[0].color = (color_t){255, 255, 255, 255};
    hurricane->attrs[1].type = ELEMENT_ATTR_NONE;

    hurricane->direction = source->direction;

    move_over_ground(&hurricane->transform.position, &source->direction, &move_def, 2.0f);
}

void hurricane_damage_targets(hurricane_t* hurricane) {
    for (
        contact_t* contact = hurricane->trigger.active_contacts; 
        contact; 
        contact = contact->next) {
        
        dynamic_object_t* obj = collision_scene_find_object(contact->other_object);

        if (obj) {
            struct Vector2 offset;
            offset.x = hurricane->transform.position.x - obj->position->x;
            offset.y = hurricane->transform.position.z - obj->position->z;
            vector2Normalize(&offset, &offset);

            float offset_x = offset.x;
            offset.x += offset.y * TANGENT_FORCE_RATIO;
            offset.y += offset_x * -TANGENT_FORCE_RATIO;

            obj->velocity.x *= 0.9f;
            obj->velocity.z *= 0.9f;
            obj->velocity.x += PUSH_FORCE * offset.x * fixed_time_step;
            if (obj->position->y < hurricane->transform.position.y) {
                obj->velocity.y -= (GRAVITY_CONSTANT * 1.2f) * fixed_time_step;
            }
            obj->velocity.z += PUSH_FORCE * offset.y * fixed_time_step;
        }

        health_t* health = health_get(contact->other_object);

        if (health) {
            health_damage(health, &hurricane_damage);
        }
    }
}

bool hurricane_update(hurricane_t* hurricane) {
    move_over_ground(&hurricane->transform.position, &hurricane->direction, &move_def, 2.0f);

    vector2ComplexMul(&hurricane->transform.rotation, &hurricane_rotate_rate, &hurricane->transform.rotation);

    hurricane->timer += fixed_time_step;

    hurricane_damage_targets(hurricane);

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
    collision_scene_remove_trigger(&hurricane->trigger);
}