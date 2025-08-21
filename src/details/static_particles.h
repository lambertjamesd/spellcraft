#ifndef __DETAIL_STATIC_PARTICLES_H__
#define __DETAIL_STATIC_PARTICLES_H__

#include <t3d/tpx.h>
#include "../math/vector3.h"

struct static_particles {
    struct Vector3 center;
    TPXParticle* particles;
    int particle_count;
};

typedef struct static_particles static_particles_t;

void static_particles_init(static_particles_t* static_particles, struct Vector3* center, TPXParticle* particles, int particle_count);
void static_particles_destroy(static_particles_t* static_particles);

#endif