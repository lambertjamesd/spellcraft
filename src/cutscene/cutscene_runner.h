#ifndef __CUTSCENE_CUTSCENE_RUNNER_H__
#define __CUTSCENE_CUTSCENE_RUNNER_H__

#include "cutscene.h"
#include "../scene/world_definition.h"
#include <stdbool.h>

typedef void (*cutscene_finish_callback)(struct cutscene* cutscene, void* data);

void cutscene_runner_init();

void cutscene_runner_run(struct cutscene* cutscene, cutscene_finish_callback finish_callback, void* data);
bool cutscene_runner_is_running();

void cutscene_runner_free_on_finish(struct cutscene* cutscene, void* data);

#endif