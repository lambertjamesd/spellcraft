#ifndef __SWORD_TRAIL_H__
#define __SWORD_TRAIL_H__

#include <stdint.h>
#include "../math/vector3.h"
#include <libdragon.h>

#define MAX_SWORD_TRAIL_LENGTH 8

struct sword_trail {
    struct Vector3 trail_halves[MAX_SWORD_TRAIL_LENGTH][2];

    uint8_t last_vertex;
    uint8_t vertex_count;

    uint32_t color;
};

struct sword_trail* sword_trail_new(color_t color);
// NULL indicates the trial is done
void sword_trail_move(struct sword_trail* trail, struct Vector3* a, struct Vector3* b);
void sword_trail_stop(struct sword_trail* trail);


#endif