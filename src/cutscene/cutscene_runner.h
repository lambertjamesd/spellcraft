#ifndef __CUTSCENE_CUTSCENE_RUNNER_H__
#define __CUTSCENE_CUTSCENE_RUNNER_H__

#include "cutscene.h"
#include "../scene/world_definition.h"
#include <stdbool.h>

void cutscene_runner_init();

void cutscene_runner_run(struct cutscene* cutscene);
bool cutscene_runner_is_running();

#endif