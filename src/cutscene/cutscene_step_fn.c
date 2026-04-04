#include "cutscene_step_fn.h"
#include "../menu/hud.h"
#include "../scene/scene.h"
#include "../menu/dialog_box.h"

void cutscene_say_ask_init(cutscene_runner_context_t* context, int arg_count) {
    assert(arg_count == 1);
}

bool cutscene_say_step(cutscene_runner_context_t* context) {
    int has_shown = context->stack_depth == evaluation_context_stack_size(&context->eval);

    if (!has_shown && !dialog_box_is_active()) {
        char* message = (char*)evaluation_context_pop(&context->eval);
        dialog_box_show(message, NULL, NULL, NULL);
    } else if (has_shown) {
        return !dialog_box_is_active();
    }

    return false;
}

bool cutscene_ask_step(cutscene_runner_context_t* context) {
    int has_shown = context->stack_depth == evaluation_context_stack_size(&context->eval);

    if (!has_shown && !dialog_box_is_active()) {
        char* message = (char*)evaluation_context_pop(&context->eval);
        dialog_box_ask(message, NULL, NULL, NULL);
    } else if (has_shown && !dialog_box_is_active()) {
        evaluation_context_push(&context->eval, dialog_box_get_response());
        return true;
    }

    return false;
}

void cutscene_show_boss_health_init(cutscene_runner_context_t* context, int arg_count) {
    assert(arg_count == 2);
    int args[2];
    evaluation_context_popn(&context->eval, args, 2);

    hud_show_boss_health(&current_scene->hud, (char*)args[0], cutscene_context_get_translate_entity(context, args[1]));
}

static cutscene_step_fn_t function_steps[] = {
    {.init = cutscene_say_ask_init, .step = cutscene_say_step}, // func say(message: str)
    {.init = cutscene_say_ask_init, .step = cutscene_ask_step}, // func ask(message: str): bool
    {.init = cutscene_show_boss_health_init}, // func show_boss_health(name: str, boss_entity: entity_id)
};

cutscene_step_fn_t* cutscene_step_lookup_fn(int type) {
    assert(type >= 0 && type < sizeof(function_steps) / sizeof(*function_steps));
    return &function_steps[type];
}