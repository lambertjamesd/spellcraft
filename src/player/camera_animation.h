#ifndef __SCENE_CAMERA_ANIMATION_H__
#define __SCENE_CAMERA_ANIMATION_H__

#include <stdint.h>
#include <stdio.h>
#include "../math/vector3.h"
#include "../math/vector2s16.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"

typedef struct cutscene cutscene_t;

struct camera_animation_frame {
    struct Vector3 position;
    float rotation[3];
    float fov;
};

struct camera_animation {
    char* name;
    uint16_t frame_count;
    uint16_t event_size;
    uint32_t rom_offset;
    uint32_t event_offset;
};

typedef struct camera_animation camera_animation_t;

enum camera_event_type {
    CAMERA_EVENT_TYPE_END,
    CAMERA_EVENT_TYPE_RUN,
    CAMERA_EVENT_SIGNAL,
    CAMERA_EVENT_WAIT,
};

typedef enum camera_event_type camera_event_type_t;

union camera_event_data {
    cutscene_t* run;
    char* signal;
    char* wait;
};

typedef union camera_event_data camera_event_data_t;

struct camera_event {
    uint8_t type;
    uint8_t unused;
    uint16_t frame;
    camera_event_data_t data;
};

typedef struct camera_event camera_event_t;

struct camera_animation_list {
    struct camera_animation* animations;
    uint16_t animation_count;
    uint32_t rom_location;
};

typedef struct camera_animation_list camera_animation_list_t;

void camera_animation_list_init(struct camera_animation_list* list, int count, uint32_t rom_location);
void camera_animation_list_destroy(struct camera_animation_list* list);

struct camera_animation* camera_animation_lookup(struct camera_animation_list* list, const char* name);

void camera_animations_load(struct camera_animation_list* list, const char* filename, FILE* file);

camera_event_t* camera_anim_load_events(camera_animation_t* anim);
void camera_anim_free_events(camera_event_t* events);

camera_event_t* camera_anim_run_events(camera_event_t* curr, uint16_t frame);

void camera_animation_map(camera_animation_list_t* list, entity_id id);
void camera_animation_unmap(entity_id id);
camera_animation_list_t* camera_animation_get(entity_id id);

#endif