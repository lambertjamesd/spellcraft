#ifndef __OVERWORLD_LOAD_H__
#define __OVERWORLD_LOAD_H__

#include "overworld.h"
#include <stdio.h>

struct overworld_tile* overworld_tile_load(FILE* file);
void overworld_tile_free(struct overworld_tile* tile);

#endif