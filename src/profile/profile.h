#ifndef __PROFILE_PROFILE_H__
#define __PROFILE_PROFILE_H__

#define ENABLE_PROFILE_main                 0
#define ENABLE_PROFILE_update               0
#define ENABLE_PROFILE_scene                0
#define ENABLE_PROFILE_render               0
#define ENABLE_PROFILE_render_particles     0
#define ENABLE_PROFILE_overworld_update     0

void profile_start();
void profile_end(const char* name);

#define SC_PROFILE_ENABLED(group) ENABLE_PROFILE_##group
#define SC_PROFILE_START(group)   if (SC_PROFILE_ENABLED(group)) profile_start()
#define SC_PROFILE_END(group, label)   if (SC_PROFILE_ENABLED(group)) profile_end(#group " " #label)

#define SC_LOOP_PROFILE_INIT(group, count)  uint64_t __profile_samples_##group[count] = {}
#define SC_LOOP_PROFILE_START(group, index) if (SC_PROFILE_ENABLED(group)) __profile_samples_##group[index] -= get_ticks_us()
#define SC_LOOP_PROFILE_END(group, index) if (SC_PROFILE_ENABLED(group)) __profile_samples_##group[index] += get_ticks_us()

#define SC_LOOP_COUNT_START(group) int __profile_sample_count##group = 0
#define SC_LOOP_INCREMENT(group)    if (SC_PROFILE_ENABLED(group)) ++__profile_sample_count##group
#define SC_LOOP_COUNT_FINISH(group)  if (SC_PROFILE_ENABLED(group)) debugf(#group " count: %d\n", __profile_sample_count##group)

#define SC_LOOP_FINISH(group, index, label) if (SC_PROFILE_ENABLED(group)) debugf(#label ": %f\n", (__profile_samples_##group[index]) * (1.0f / 1000.0f))

#endif