#include "training_dummy.h"

#include "../collision/collision_scene.h"
#include "../render/render_scene.h"

static struct dynamic_object_type dummy_collision = {
    .minkowsi_sum = dynamic_object_capsule_minkowski_sum,
    .bounding_box = dynamic_object_capsule_bounding_box,
    .data = {
        .capsule = {
            .radius = 0.25f,
            .inner_half_height = 0.5f,
        }
    }
};

void training_dummy_init(struct training_dummy* dummy, struct training_dummy_definition* definition) {
    entity_id entity_id = entity_id_new();

    transformInitIdentity(&dummy->transform);
    dummy->transform.position = definition->position;
    quatAxisComplex(&gUp, &definition->rotation, &dummy->transform.rotation);

    renderable_init(&dummy->renderable, &dummy->transform, "rom:/meshes/objects/training_dummy.tmesh");

    render_scene_add_renderable(&dummy->renderable, 2.0f);

    dynamic_object_init(
        entity_id,
        &dummy->collision,
        &dummy_collision,
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_DAMAGE_ENEMY,
        &dummy->transform.position,
        NULL
    );

    dummy->collision.center.y = dummy_collision.data.capsule.inner_half_height + dummy_collision.data.capsule.radius;

    collision_scene_add(&dummy->collision);

    dummy->collision.is_fixed = true;
}

void training_dummy_destroy(struct training_dummy* dummy) {
    renderable_destroy(&dummy->renderable);
    collision_scene_remove(&dummy->collision);
}