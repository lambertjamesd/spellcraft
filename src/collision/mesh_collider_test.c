#include "mesh_collider.h"

#include "../test/framework_test.h"

bool mesh_triangle_shadow_cast(struct mesh_triangle_indices indices, struct Vector3* vertices, struct Vector3* starting_point, struct mesh_shadow_cast_result* result);

void test_mesh_triangle_shadow_cast(struct test_context* t) {
    struct Vector3 vertices[] = {
        {-8.203972f, -20.766230f, 152.183746f},
        {-6.987402f, -20.767448f, 156.924408f},
        {-5.042764f, -20.766230f, 155.332092f},
    };

    struct mesh_triangle_indices indices = {
        .indices = {0, 1, 2},
        .surface_type = 0,
    };

    struct Vector3 starting_point = {-6.948174f, -20.766401f, 153.931503f};

    struct mesh_shadow_cast_result result;

    if (!mesh_triangle_shadow_cast(indices, vertices, &starting_point, &result)) {
        test_fatal(t, "expected shadow cast to hit triangle");
    }

    test_gtf(t, result.y, -20.7674f);
    test_ltf(t, result.y, -20.7654f);
    test_gtf(t, result.normal.y, 0.0f);
    test_eqi(t, 0, result.surface_type);
}
