#ifndef __WATER_WATER_H__
#define __WATER_WATER_H__

#include <stdint.h>
#include <t3d/t3d.h>
#include "../math/vector2s16.h"
#include "../render/tmesh.h"

void water_simulation_retain();
void water_simulation_release();

void water_simulation_enable_debug_render();
void water_simulation_disable_debug_render();

void water_simulation_update();

void water_simulation_apply(tmesh_t* mesh, vector3_t* position);

void water_simulation_set_center(vector3_t* position);
void water_simulation_set(vector3_t* position, int8_t value, int8_t radius);

#endif