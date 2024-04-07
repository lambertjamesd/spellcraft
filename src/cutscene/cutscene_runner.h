#ifndef __CUTSCENE_CUTSCENE_RUNNER_H__
#define __CUTSCENE_CUTSCENE_RUNNER_H__

#include "cutscene.h"
#include "../scene/world_definition.h"
#include <stdbool.h>

void cutscene_runner_init();

void cutscene_runner_queue_step(struct cutscene_step* step);

void cutscene_runner_pause(bool should_pause, bool should_change_game_mode);
void cutscene_runner_dialog(char* message);
void cutscene_runner_show_rune(enum spell_symbol_type rune, bool should_show);

void cutscene_runner_run(struct cutscene* cutscene);
bool cutscene_runner_is_running();

#endif