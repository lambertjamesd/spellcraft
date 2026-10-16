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
    vector3s16_t velocity;
};

struct newton_particles {
    TPXParticleS16* particles;
    vector3s16_t accel;
};

typedef struct newton_particles newton_particles_t;

#endif