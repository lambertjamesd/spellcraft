#ifndef __OBJECTS_LINE_MESH_H__
#define __OBJECTS_LINE_MESH_H__

#include "../math/vector3.h"
#include "../scene/scene_definition.h"
#include "../entity/entity_id.h"

struct line_mesh_edge {
    uint8_t a;
    uint8_t next_edge_a;
    uint8_t b;
    uint8_t next_edge_b;
};

typedef struct line_mesh_edge line_mesh_edge_t;

struct line_mesh {
    struct Vector3 position;
    uint8_t point_count;
    uint8_t line_count;
    struct Vector3* points;
    line_mesh_edge_t* edges;
};

typedef struct line_mesh line_mesh_t;

void line_mesh_init(line_mesh_t* line_mesh, struct line_mesh_definition* definition, entity_id entity_id);
void line_mesh_destroy(line_mesh_t* line_mesh, struct line_mesh_definition* definition);
void line_mesh_common_init();
void line_mesh_common_destroy();

#endif