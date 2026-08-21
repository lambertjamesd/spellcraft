#ifndef __SCREEN_RAYCAST_SCREEN_RAYCAST_H__
#define __SCREEN_RAYCAST_SCREEN_RAYCAST_H__

#include <stdint.h>

void screen_raycast_init();
void screen_raycast_destroy();

void screen_raycast_check_pixel(void* pos);
void screen_raycast_check_changed(void* pos, int id);

struct screen_raycast_result {
    int id;
    int pixel_value;
} __attribute__((aligned(8)));

typedef struct screen_raycast_result screen_raycast_result_t;

void screen_raycast_read_entity(screen_raycast_result_t* result);

#endif