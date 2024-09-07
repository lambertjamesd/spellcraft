#include "fire.h"

#include "assets.h"
#include "../collision/collision_scene.h"
#include "../collision/shapes/cone.h"
#include "../entity/health.h"
#include "../math/mathf.h"
#include "../render/render_scene.h"
#include "../time/time.h"

#define CYCLE_TIME  0.08f

#define FIRE_LENGTH         4.0f

#define MAX_RADIUS          0.5f
#define MAX_RANDOM_OFFSET   0.3f

#define START_FADE          0.75f

#define TIP_RISE            0.5f

static struct dynamic_object_type fire_object_type = {
    .minkowsi_sum = cone_minkowski_sum,
    .bounding_box = cone_bounding_box,
    .data = { 
        .cone = {
            .size = {MAX_RADIUS, MAX_RADIUS, FIRE_LENGTH},
        },
    },
};

void fire_apply_transform(struct fire* fire) {
    fire->transform.position = fire->data_source->position;

    fire->transform.rotation.x = -fire->data_source->direction.z;
    fire->transform.rotation.y = -fire->data_source->direction.x;

    vector2Normalize(&fire->transform.rotation, &fire->transform.rotation);
}

void fire_init(struct fire* fire, struct spell_data_source* source, struct spell_event_options event_options, enum element_type element_type) {
    fire->data_source = source;
    spell_data_source_retain(source);

    fire_apply_transform(fire);

    fire->flame_effect = scale_in_fade_out_new(spell_assets_get()->fire_sweep_mesh, &fire->transform.position, &fire->transform.rotation, FIRE_LENGTH);

    dynamic_object_init(
        entity_id_new(), 
        &fire->dynamic_object, 
        &fire_object_type, 
        COLLISION_LAYER_DAMAGE_ENEMY,
        &fire->transform.position, 
        &fire->transform.rotation
    );
    fire->dynamic_object.is_trigger = 1;
    fire->element_type = element_type;
    collision_scene_add(&fire->dynamic_object);
}

void fire_destroy(struct fire* fire) {
    spell_data_source_release(fire->data_source);
    collision_scene_remove(&fire->dynamic_object);
    scale_in_fade_out_stop(fire->flame_effect);
}

void fire_apply_damage(struct dynamic_object* dyanmic_object, enum damage_type damage_type) {
    struct contact* curr = dyanmic_object->active_contacts;

    while (curr) {
        struct health* target_health = health_get(curr->other_object);
        curr = curr->next;

        if (!target_health) {
            continue;
        }

        health_damage(target_health, 1.0f, dyanmic_object->entity_id, damage_type);
    }
}

enum damage_type fire_determine_damage_type(enum element_type element_type) {
    switch (element_type) {
        case ELEMENT_TYPE_FIRE:
            return DAMAGE_TYPE_FIRE;
        case ELEMENT_TYPE_ICE:
            return DAMAGE_TYPE_ICE;
        case ELEMENT_TYPE_LIGHTNING:
            return DAMAGE_TYPE_LIGHTING;
        default:
            return 0;
    }
}

void fire_update(struct fire* fire, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    if (fire->data_source->flags.cast_state != SPELL_CAST_STATE_ACTIVE) {
        spell_event_listener_add(event_listener, SPELL_EVENT_DESTROY, NULL, 0.0f);
    }

    fire_apply_transform(fire);

    scale_in_fade_out_set_transform(fire->flame_effect, &fire->transform.position, &fire->transform.rotation);

    fire_apply_damage(&fire->dynamic_object, fire_determine_damage_type(fire->element_type));
}