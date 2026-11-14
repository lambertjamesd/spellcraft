#include "newton_particles.h"

#include "../time/time.h"

// multiplies 8.8 * .16 for a 11.5 result
#define SCALE_VEL(vel, dt)  (int16_t)(((int32_t)vel * (uint32_t)dt) >> 21)
// multiplies 8.8 * .16 for a 8.8 result
#define SCALE_ACC(acc, dt)  (int16_t)(((int32_t)acc * (uint32_t)dt) >> 16)

void newton_particles_apply_frame_single(newton_particles_t* particles, TPXParticleS16* particle, bool is_b, newton_particle_t* particle_physics, uint16_t dt) {
    uint8_t* curr = (uint8_t*)particles->keyframe_data + particle_physics->current_frame;

    int16_t* pos = is_b ? particle->posB : particle->posA;

    pos[0] += SCALE_VEL(particle_physics->velocity.x, dt);
    pos[1] += SCALE_VEL(particle_physics->velocity.y, dt);
    pos[2] += SCALE_VEL(particle_physics->velocity.z, dt);
    
    particle_physics->velocity.x += SCALE_ACC(particles->accel.x, dt);
    particle_physics->velocity.y += SCALE_ACC(particles->accel.y, dt);
    particle_physics->velocity.z += SCALE_ACC(particles->accel.z, dt);

    if (particle_physics->delay_frames) {
        --particle_physics->delay_frames;
        return;
    }

    while (*curr != KEYFRAME_COMMAND_END) {
        switch (*curr)
        {
        case KEYFRAME_COMMAND_FRAME_END:
            curr += 1;
            break;
        case KEYFRAME_COMMAND_COLOR: {
            uint8_t* color = is_b ? particle->colorB : particle->colorA;

            color[0] += curr[1];
            color[1] += curr[2];
            color[2] += curr[3];
            color[3] += curr[4];

            curr += 5;
            break;
        }
        case KEYFRAME_COMMAND_SIZE: {
            if (is_b) {
                particle->sizeB += curr[1];
            } else {
                particle->sizeA += curr[1];
            }

            curr += 2;
            break;
        }
        case KEYFRAME_COMMAND_DELAY: {
            particle_physics->delay_frames = curr[1];
            curr += 2;
            break;
        }
        case KEYFRAME_COMMAND_TEX_OFFSET: {
            if (is_b) {
                particle->texOffsetB = curr[1];
            } else {
                particle->texOffsetA = curr[1];
            }

            curr += 2;
            break;
        }
        default:
            assert(false);
            break;
        }
    }

    particle_physics->current_frame = curr - (uint8_t*)particles->keyframe_data;
}

void newton_particles_apply_frame(newton_particles_t* particles) {
    TPXParticleS16* curr_particle = particles->particles;

    uint16_t dt = (uint16_t)(0x10000 * scaled_time_step);

    for (int i = 0; i < particles->particle_count; i += 2) {
        newton_particles_apply_frame_single(particles, curr_particle, false, &particles->particle_physics[i], dt);
        newton_particles_apply_frame_single(particles, curr_particle, true, &particles->particle_physics[i + 1], dt);
        curr_particle += 1;
    }
}