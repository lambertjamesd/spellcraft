#ifndef __TIME_TIME_H__
#define __TIME_TIME_H__

typedef void (*update_callback)(void* data);

typedef int update_id;

void update_reset();
update_id update_add(void* data, update_callback callback, int priority, int mask);
void update_remove(update_id id);

void update_dispatch(int mask);

float fixed_time_step;

#endif