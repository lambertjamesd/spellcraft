#include "cutscene_step_fn.h"
#include "../menu/hud.h"
#include "../scene/scene.h"

void cutscene_show_boss_health_init(cutscene_runner_context_t* context, void* active_step_data, int arg_count) {
    assert(arg_count == 2);
    int args[2];
    evaluation_context_popn(&context->eval, args, 2);

    hud_show_boss_health(&current_scene->hud, (char*)&args[0], cutscene_context_get_translate_entity(context, args[1]));
}

static cutscene_step_fn_t function_steps[] = {
    {.init = cutscene_show_boss_health_init}, // func show_boss_health(name: str, boss_entity: entity_id)
};