#include "elemental_sword.h"

#include "../render/render_scene.h"
#include "../resource/tmesh_cache.h"
#include "../math/transform.h"
#include "../collision/collision_scene.h"
#include "../entity/health.h"

static struct elemental_sword_definition swing_definitions[] = {
    [ELEMENT_TYPE_FIRE] = {
        .damage_source = {
            .amount = 10.0f,
            .type = DAMAGE_TYPE_FIRE | DAMAGE_TYPE_KNOCKBACK,
            .knockback_strength = 1.0f,
        },
        .animation = SPELL_ANIMATION_SWING,
        .sword_length = 1.0f,
        .mana_cost = 1.0f,
    }
};

static struct elemental_sword_definition spin_definitions[] = {
    [ELEMENT_TYPE_FIRE] = {
        .damage_source = {
            .amount = 10.0f,
            .type = DAMAGE_TYPE_FIRE | DAMAGE_TYPE_KNOCKBACK,
            .knockback_strength = 1.0f,
        },
        .animation = SPELL_ANIMATION_SPIN,
        .sword_length = 1.5f,
        .mana_cost = 3.0f,
    }
};

void elemental_sword_render(void* data, struct render_batch* batch) {
    struct elemental_sword* sword = (struct elemental_sword*)data;

    if (sword->data_source->flags.cast_state != SPELL_CAST_STATE_ACTIVE) {
        return;
    }

    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    struct Transform transform;
    transform.position = sword->data_source->position;
    quatLook(&sword->data_source->direction, &gUp, &transform.rotation);
    transform.scale = gOneVec;

    mat4x4 mtx;
    transformToWorldMatrix(&transform, mtx);
    render_batch_relative_mtx(batch, mtx);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    render_batch_add_tmesh(batch, sword->mesh, mtxfp, 1, NULL, NULL);
}

void elemental_sword_init(struct elemental_sword* elemental_sword, struct spell_data_source* source, struct spell_event_options event_options, enum element_type element_type) {
    entity_id entity_id = entity_id_new();
    elemental_sword->data_source = spell_data_source_retain(source);
    elemental_sword->definition = event_options.modifiers.earthy ?
        &spin_definitions[element_type] :
        &swing_definitions[element_type];

    spell_data_source_request_animation(source, elemental_sword->definition->animation);

    render_scene_add(&source->position, 1.0f, elemental_sword_render, elemental_sword);

    elemental_sword->mesh = tmesh_cache_load("rom:/meshes/spell/fire_sword.tmesh");

    elemental_sword->trail = sword_trail_new();

    elemental_sword->collider_type = (struct dynamic_object_type){
        .minkowsi_sum = swing_shape_minkowski_sum,
        .bounding_box = swing_shape_bounding_box,
        .data = {
            .swing = {
                .shape = &elemental_sword->swing_shape,
            },
        },
    };
    swing_shape_init(&elemental_sword->swing_shape);

    dynamic_object_init(
        entity_id, 
        &elemental_sword->collider, 
        &elemental_sword->collider_type, 
        COLLISION_LAYER_DAMAGE_ENEMY,
        &gZeroVec,
        NULL
    );

    elemental_sword->collider.trigger_type = TRIGGER_TYPE_OVERLAP;

    collision_scene_add(&elemental_sword->collider);

    damaged_set_reset(&elemental_sword->damaged_set);

    elemental_sword->needs_mana_check = 1;
    elemental_sword->power_ratio = 0.0f;
}

void elemental_sword_destroy(struct elemental_sword* elemental_sword) {
    spell_data_source_release(elemental_sword->data_source);
    tmesh_cache_release(elemental_sword->mesh);
    render_scene_remove(elemental_sword);
    sword_trail_stop(elemental_sword->trail);
    collision_scene_remove(&elemental_sword->collider);
}

bool elemental_sword_update(struct elemental_sword* elemental_sword, struct spell_event_listener* event_listener, struct spell_sources* spell_sources) {
    if (elemental_sword->needs_mana_check) {
        elemental_sword->power_ratio = mana_pool_request_ratio(&spell_sources->mana_pool, elemental_sword->definition->mana_cost);
        elemental_sword->needs_mana_check = 0;
    }

    if (elemental_sword->power_ratio > 0.0f && elemental_sword->data_source->flags.cast_state == SPELL_CAST_STATE_ACTIVE) {
        struct Vector3 tip;
        vector3AddScaled(
            &elemental_sword->data_source->position, 
            &elemental_sword->data_source->direction, 
            elemental_sword->definition->sword_length * elemental_sword->power_ratio, 
            &tip
        );
        sword_trail_move(elemental_sword->trail, &elemental_sword->data_source->position, &tip);
        swing_shape_add(&elemental_sword->swing_shape, &elemental_sword->data_source->position, &tip);

        health_apply_contact_damage(&elemental_sword->collider, &elemental_sword->definition->damage_source, &elemental_sword->damaged_set);
    }

    return elemental_sword->data_source->flags.is_animating;
}