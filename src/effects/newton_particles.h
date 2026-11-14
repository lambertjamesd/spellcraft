#ifndef __EFFECTS_NEWTON_PARTICLES_H__
#define __EFFECTS_NEWTON_PARTICLES_H__

#include <t3d/tpx.h>
#include "../math/vector3s16.h"
#include "../math/vector3.h"

enum particle_emitter_shape_type {
    PARTICLE_EM_SHAPE_SPHERE,
    PARTICLE_EM_SHAPE_CUBE,
    PARTICLE_EM_SHAPE_CYLINDER,
    PARTICLE_EM_SHAPE_CIRCLE,
};

typedef enum particle_emitter_shape_type particle_emitter_shape_type_t;

struct particle_emitter_shape {
    particle_emitter_shape_type_t type;
    union {
        struct {
            float radius;
        } sphere;
        struct {
            vector3_t half_size;
        } cube;
        struct {
            float radius;
            float half_height;
        } cylinder;
        struct {
            float radius;
        } circle;
    };
};

struct newton_particle {
    // fixed point 8.8
    vector3s16_t velocity;
    uint16_t current_frame;
    uint8_t delay_frames;
};

typedef struct newton_particle newton_particle_t;

enum keyframe_command {
    KEYFRAME_COMMAND_END,
    KEYFRAME_COMMAND_FRAME_END,
    KEYFRAME_COMMAND_COLOR,
    KEYFRAME_COMMAND_DELAY,
    KEYFRAME_COMMAND_SIZE,
    KEYFRAME_COMMAND_TEX_OFFSET,
};

typedef enum keyframe_command keyframe_command_t;

struct newton_particles {
    TPXParticleS16* particles;
    newton_particle_t* particle_physics;
    uint16_t particle_count;
    uint16_t next_particle;
    int8_t starting_size;
    // fixed point 8.8
    vector3s16_t accel;
    vector3s16_t min_start_vel;
    vector3s16_t max_start_vel;
    color_t starting_color;
    void* keyframe_data;
};

typedef struct newton_particles newton_particles_t;

#endif