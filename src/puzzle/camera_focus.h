#ifndef __PUZZLE_CAMERA_FOCUS_H__
#define __PUZZLE_CAMERA_FOCUS_H__

#include "../math/vector3.h"
#include "../math/quaternion.h"
#include "../scene/scene_definition.h"
#include "../entity/entity_id.h"

struct camera_focus {
    struct Vector3 position;
    struct Quaternion rotation;
    float fov;
    boolean_variable input;
    boolean_variable output;
    bool repeat;
    bool last_state;
    bool did_fire;
};

typedef struct camera_focus camera_focus_t;

void camera_focus_init(camera_focus_t* camera_focus, struct camera_focus_definition* definition, entity_id entity_id);
void camera_focus_destroy(camera_focus_t* camera_focus);
void camera_focus_common_init();
void camera_focus_common_destroy();

#endif