#ifndef __EFFECTS_ITEM_GET_CUTSCENE_H__
#define __EFFECTS_ITEM_GET_CUTSCENE_H__

#include "../render/renderable.h"
#include "../cutscene/cutscene.h"

typedef void (*item_get_finish)(void* data);

void item_get_cutscene_run(cutscene_builder_t* builder, const char* message, renderable_t* renderable, item_get_finish on_finish, void* data); 

#endif