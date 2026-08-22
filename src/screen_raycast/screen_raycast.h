#ifndef __SCREEN_RAYCAST_SCREEN_RAYCAST_H__
#define __SCREEN_RAYCAST_SCREEN_RAYCAST_H__

#include <stdint.h>

void screen_raycast_check_before(void* pos);
void screen_raycast_check_after(int id);

void screen_raycast_read_result(int* output);

#endif