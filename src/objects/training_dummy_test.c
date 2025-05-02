#include "training_dummy.h"
#include "../test/framework_test.h"
#include "../collision/collision_scene.h"
#include "../collision/shapes/sphere.h"

#include "../time/time.h"

static struct dynamic_object_type test_projectile_collision = {
    .minkowsi_sum = sphere_minkowski_sum,
    .bounding_box = sphere_bounding_box,
    .data = {
        .sphere = {
            .radius = 0.25f,
        }
    },
    .bounce = 0.4f,
    .friction = 0.25f,
};

void test_training_dummy(struct test_context* t) {
    struct training_dummy dummy;
    struct training_dummy_definition definition = (struct training_dummy_definition){
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f},
    };

    training_dummy_init(&dummy, &definition);

    entity_id damage_source = entity_id_new();
    struct Vector3 projectile_pos = {0.0f, 1.0f, -1.0f};
    struct dynamic_object projectile;
    dynamic_object_init(damage_source, &projectile, &test_projectile_collision, ~0, &projectile_pos, NULL);
    collision_scene_add(&projectile);

    test_near_equalf(t, 0.0f, dummy.angularVelocity.x);
    test_near_equalf(t, 0.0f, dummy.angularVelocity.y);
    test_near_equalf(t, 0.0f, dummy.angularVelocity.z);

    struct damage_info damage = {
        .amount = 1.0f,
        .type = DAMAGE_TYPE_PROJECTILE,
        .source = damage_source,
        .direction = { 0.0f, 0.0f, 1.0f },
    };

    health_damage(&dummy.health, &damage);

    test_gtf(t, dummy.angularVelocity.x, 0.0f);
    test_near_equalf(t, 0.0f, dummy.angularVelocity.y);
    test_near_equalf(t, 0.0f, dummy.angularVelocity.z);

    float prev_angular_velocity = dummy.angularVelocity.x;

    struct Vector3 dummy_up;
    quatMultVector(&dummy.transform.rotation, &gUp, &dummy_up);
    float prev_alignment = vector3Dot(&gUp, &dummy_up);
    test_near_equalf(t, 1.0f, prev_alignment);

    update_dispatch();

    quatMultVector(&dummy.transform.rotation, &gUp, &dummy_up);
    test_ltf(t, vector3Dot(&gUp, &dummy_up), prev_alignment);

    update_dispatch();

    test_ltf(t, fabsf(dummy.angularVelocity.x), fabsf(prev_angular_velocity));

    collision_scene_remove(&projectile);
    training_dummy_destroy(&dummy);
}