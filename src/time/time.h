#ifndef __TIME_TIME_H__
#define __TIME_TIME_H__

typedef void (*update_callback)(void* data);

typedef int update_id;

#define UPDATE_LAYER_WORLD  (1 << 0)

#define UPDATE_PRIORITY_PLAYER  0
#define UPDATE_PRIORITY_SPELLS  1
#define UPDATE_PRIORITY_CAMERA  2

void update_reset();
update_id update_add(void* data, update_callback callback, int priority, int mask);
void update_remove(update_id id);
void update_remove_with_data(void* data, update_callback callback);

void update_dispatch(int mask);

extern float fixed_time_step;

#endif