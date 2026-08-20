#ifndef __REPAIR_REPAIR_PART_H__
#define __REPAIR_REPAIR_PART_H__

#include "../render/tmesh.h"
#include "../math/transform.h"
#include "../collision/mesh_collider.h"
#include "../render/frame_alloc.h"
#include "../math/ray.h"
#include "../scene/scene_definition.h"
#include <stdio.h>

struct repair_old_collider {
    vector3_t* vertices;
    uint16_t* indices;
    uint16_t triangle_count;
};

typedef struct repair_old_collider repair_old_collider_t;

struct repair_old_part {
    transform_t transform;
    tmesh_t mesh;
    tmesh_t solid_mesh;
    repair_old_collider_t collider;
    quaternion_t target_rotation;

    vector3_t end_position;
    quaternion_t end_rotation;

    boolean_variable has_part;

    bool is_connected;
    bool is_present;
    bool prevent_rotation;
    int8_t depends_on;
    int8_t blocks;
};

typedef struct repair_old_part repair_old_part_t;

void repair_old_part_load(repair_old_part_t* part, FILE* file);
void repair_old_part_destroy(repair_old_part_t* part);

void repair_old_part_render(repair_old_part_t* part, struct frame_memory_pool* pool);
void repair_old_part_render_drop_location(repair_old_part_t* part, struct frame_memory_pool* pool);

bool repair_old_part_raycast(repair_old_part_t* part, ray_t* ray, float* distance);
void repair_old_part_update(repair_old_part_t* part);
void repair_old_part_set_complete(repair_old_part_t* part);

#endif