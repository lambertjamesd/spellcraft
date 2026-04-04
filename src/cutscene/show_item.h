#ifndef __CUTSCENE_SHOW_RUNE_H__
#define __CUTSCENE_SHOW_RUNE_H__

#include <stdint.h>
#include <stdbool.h>
#include "cutscene.h"
#include "../render/material.h"
#include "../scene/scene_definition.h"

void show_item_start(enum inventory_item_type item, bool should_show);
bool show_item_update();
void show_item_render();
void show_item_cleanup();

void show_item_in_cutscene(struct cutscene_builder* cutscene_builder, enum inventory_item_type item);

#endif