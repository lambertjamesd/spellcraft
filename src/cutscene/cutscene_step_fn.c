#include "cutscene_step_fn.h"
#include "../menu/hud.h"
#include "../scene/scene.h"
#include "../menu/dialog_box.h"
#include "../time/time.h"

// say/ask
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

// show_item

void cutscene_show_item(cutscene_runner_context_t* context, int arg_count) {

}

// pause

void cutscene_step_pause(cutscene_runner_context_t* context, int arg_count) {
    assert(arg_count == 0);
    update_pause_layers(UPDATE_LAYER_WORLD);
}

void cutscene_step_unpause(cutscene_runner_context_t* context, int arg_count) {
    assert(arg_count == 0);
    update_unpause_layers(UPDATE_LAYER_WORLD);
}

// delay

void cutscene_step_delay_init(cutscene_runner_context_t* context, int arg_count) {
    assert(arg_count == 1);
}

bool cutscene_step_delay_step(cutscene_runner_context_t* context) {
    float time = evaluation_context_pop_float(&context->eval);
    time -= fixed_time_step;

    if (time <= 0.0f) {
        return true;
    }

    evaluation_context_push_float(&context->eval, time);
    return false;
}

// show_boss_health
void cutscene_show_boss_health_init(cutscene_runner_context_t* context, int arg_count) {
    assert(arg_count == 2);
    int args[2];
    evaluation_context_popn(&context->eval, args, 2);

    hud_show_boss_health(&current_scene->hud, (char*)args[0], cutscene_context_get_translate_entity(context, args[1]));
}

static cutscene_step_fn_t function_steps[] = {
    {.init = cutscene_say_ask_init, .step = cutscene_say_step}, // func say(message: str)
    {.init = cutscene_say_ask_init, .step = cutscene_ask_step}, // func ask(message: str): bool
    {.init = cutscene_step_pause }, // func pause()
    {.init = cutscene_step_unpause }, // func unpause()
    {.init = cutscene_step_delay_init, .step = cutscene_step_delay_step }, // func delay(duration: float)
    {.init = cutscene_show_boss_health_init}, // func show_boss_health(name: str, boss_entity: entity_id)
};

cutscene_step_fn_t* cutscene_step_lookup_fn(int type) {
    assert(type >= 0 && type < sizeof(function_steps) / sizeof(*function_steps));
    return &function_steps[type];
}