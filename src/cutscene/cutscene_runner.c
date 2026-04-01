#include "cutscene_runner.h"

#include "../spell/spell.h"
#include "../time/time.h"
#include "../time/game_mode.h"
#include "../math/mathf.h"
#include "../menu/menu_rendering.h"
#include "../objects/empty.h"
#include "../util/flags.h"
#include "../scene/scene.h"
#include "evaluation_context.h"
#include "expression_evaluate.h"
#include "show_item.h"
#include "../savefile/savefile.h"
#include "../effects/fade_effect.h"
#include "../effects/area_title.h"
#include "expression_evaluate.h"
#include "../menu/dialog_box.h"
#include "../collision/collision_scene.h"
#include "../player/inventory.h"
#include "../cutscene/cutscene_stopwatch.h"
#include "cutscene_timer.h"
#include "show_item.h"
#include <assert.h>
#include "../effects/image_overlay.h"

#define MAX_QUEUED_CUTSCENES    4
#define MAX_CUTSCENE_CALL_DEPTH 6
#define MAX_STRING_STACK_SIZE   4096

#define PACK_FN_REF(fn_index, instruction)  (((fn_index) << 16) | instruction)
#define FN_REF_GET_FN(ref)  ((ref) >> 16)
#define FN_REF_GET_INST(ref)    ((ref) & 0xFFFF)

static color_t fade_colors[] = {
    {0, 0, 0, 0},
    {0, 0, 0, 255},
    {255, 255, 255, 255},
};

extern struct scene* current_scene;

union cutscene_runner_data {
    struct { bool has_shown; } dialog;
    struct { float time; } delay;
    struct { entity_id target; } npc_wait;
};

struct cutscene_queue_entry {
    cutscene_t* cutscene;
    cutscene_finish_callback finish_callback;
    void* data;
    entity_id subject;
    uint16_t function_index;
};

struct cutscene_stack_entry {
    cutscene_t* cutscene;
    cutscene_function_t* function;
    int16_t current_instruction;
    uint16_t string_stack_position;
    uint16_t stack_position;
    uint8_t retc;
};

typedef struct cutscene_stack_entry cutscene_stack_entry_t;

#define MAX_CALL_STACK_SIZE         8
#define CUTSCENE_INACTIVE_DEPTH     -1

struct cutscene_active_entry {
    cutscene_finish_callback finish_callback;
    void* data;
    entity_id subject;
    int16_t current_depth;
    cutscene_stack_entry_t call_stack[MAX_CALL_STACK_SIZE];
    struct evaluation_context context;
    char string_stack[MAX_STRING_STACK_SIZE];
    char* current_string_start;
};

typedef struct cutscene_active_entry cutscene_active_entry_t;

#define CUTSCENE_IS_RUNNING(active_entry) ((active_entry)->current_depth != -1)
#define CUTSCENE_CURR_FRAME(active_entry)   (&(active_entry)->call_stack[(active_entry)->current_depth])

struct cutscene_runner {
    uint16_t next_cutscene;
    uint16_t last_cutscene;

    struct cutscene_queue_entry queue[MAX_QUEUED_CUTSCENES];

    cutscene_active_entry_t active_cutscene;

    show_item_t show_item;

    union cutscene_runner_data active_step_data;
};

static struct cutscene_runner cutscene_runner;

static inline int cutscene_next_queue_index(int index) {
    if (index + 1 == MAX_QUEUED_CUTSCENES) {
        return 0;
    }

    return index + 1;
}

static inline int cutscene_prev_queue_index(int index) {
    if (index == 0) {
        return MAX_QUEUED_CUTSCENES - 1;
    }

    return index - 1;
}

void cutscene_runner_start(struct cutscene* cutscene, int function_index, cutscene_finish_callback finish_callback, void* data, entity_id subject);

entity_id cutscene_runner_translate_entity(struct cutscene_active_entry* cutscene, entity_id id) {
    if (id == ENTITY_ID_SUBJECT) {
        return cutscene->subject;
    }

    return id;
}

cutscene_actor_t* cutscene_runner_lookup_actor(struct cutscene_active_entry* cutscene, entity_id input) {
    if (input == ENTITY_ID_SUBJECT) {
        return cutscene_actor_find(cutscene->subject);
    }

    return cutscene_actor_find(input);
}

vector3_t* cutscene_runner_lookup_pos(struct cutscene_active_entry* cutscene, entity_id id) {
    if (id == ENTITY_ID_SUBJECT) {
        id = cutscene->subject;
    }

    cutscene_actor_t* actor = cutscene_actor_find(id);

    if (actor != NULL) {
        return cutscene_actor_get_pos(actor);
    }

    empty_t* empty = empty_find(id);

    if (empty != NULL) {
        return &empty->position;
    }

    dynamic_object_t* obj = collision_scene_find_object(id);

    if (obj) {
        return obj->position;
    }

    return NULL;
}

void cutscene_runner_init_step(struct cutscene_active_entry* cutscene, struct cutscene_step* step) {
    switch (step->type)
    {
        case CUTSCENE_STEP_DIALOG:
        case CUTSCENE_STEP_ASK:
            cutscene_runner.active_step_data.dialog.has_shown = false;
            break;
        case CUTSCENE_STEP_SHOW_ITEM:
            show_item_start(&cutscene_runner.show_item, &step->data);
            break;
        case CUTSCENE_STEP_PAUSE:
            if (step->data.pause.should_pause) {
                if (step->data.pause.should_change_game_mode) {
                    current_game_mode = GAME_MODE_TRANSITION_TO_MENU;
                }
                update_pause_layers(step->data.pause.layers);
            } else {
                if (step->data.pause.should_change_game_mode) {
                    current_game_mode = GAME_MODE_3D;
                }
                update_unpause_layers(step->data.pause.layers);
            }
            break;
        case CUTSCENE_STEP_EXPRESSION:
            expression_evaluate(&cutscene->context, &step->data.expression.expression);
            break;
        case CUTSCENE_STEP_JUMP_IF_NOT:
        case CUTSCENE_STEP_JUMP:
            // logic is done in update step
            break;
        case CUTSCENE_STEP_SET_LOCAL: {
            assert(false);
            break;
        }
        case CUTSCENE_STEP_SET_SCENE: {
            struct evaluation_context* context = &cutscene->context;

            evaluation_context_save(
                expression_get_scene_variables(),
                step->data.store_variable.data_type,
                step->data.store_variable.word_offset,
                evaluation_context_pop(context)
            );
            break;
        }
        case CUTSCENE_STEP_SET_GLOBAL: {
            struct evaluation_context* context = &cutscene->context;

            evaluation_context_save(
                savefile_get_globals(GLOBAL_ACCESS_MODE_WRITE),
                step->data.store_variable.data_type,
                step->data.store_variable.word_offset,
                evaluation_context_pop(context)
            );
            break;
        }
        case CUTSCENE_STEP_DELAY: {
            cutscene_runner.active_step_data.delay.time = step->data.delay.duration;
            break;   
        }
        case CUTSCENE_STEP_INTERACT_WITH_NPC: {
            int entities[2];
            evaluation_context_popn(&cutscene->context, entities, 2);

            struct cutscene_actor* subject = cutscene_runner_lookup_actor(cutscene, entities[0]);
            if (!subject) {
                break;
            }

            vector3_t* target = cutscene_runner_lookup_pos(cutscene, entities[1]);
            if (!target) {
                break;
            }

            cutscene_actor_interact_with(
                subject,
                step->data.interact_with_npc.type,
                target
            );

            break;
        }
        case CUTSCENE_STEP_IDLE_NPC: {
            struct cutscene_actor* subject = cutscene_runner_lookup_actor(cutscene, evaluation_context_pop(&cutscene->context));
            if (!subject) {
                break;
            }
            cutscene_actor_idle(subject);
            break;
        }
        case CUTSCENE_STEP_CAMERA_LOOK_AT_NPC: {
            entity_id entity_id = evaluation_context_pop(&cutscene->context);

            struct cutscene_actor* target = cutscene_runner_lookup_actor(cutscene, entity_id);
            if (target) {
                camera_look_at(&current_scene->camera_controller, &target->transform.position);
                break;
            }

            dynamic_object_t* obj = collision_scene_find_object(entity_id);

            if (obj) {
                vector3_t center;
                vector3Lerp(&obj->bounding_box.min, &obj->bounding_box.max, 0.5f, &center);
                camera_look_at(&current_scene->camera_controller, &center);
                break;
            }

            empty_t* empty = empty_find(entity_id);

            if (empty) {
                camera_look_at(&current_scene->camera_controller, &empty->position);
                break;
            }
        }
        case CUTSCENE_STEP_CAMERA_FOLLOW: {
            camera_follow_player(&current_scene->camera_controller);
            break;
        }
        case CUTSCENE_STEP_CAMERA_RETURN: {
            camera_return(&current_scene->camera_controller);
            break;
        }
        case CUTSCENE_STEP_CAMERA_ANIMATE: {
            camera_play_animation(
                &current_scene->camera_controller, 
                camera_animation_lookup(&current_scene->camera_animations, step->data.camera_animate.animation_name)
            );
            break;
        }
        case CUTSCENE_STEP_CAMERA_MOVE_TO: {
            camera_move_to(&current_scene->camera_controller, &step->data.camera_move_to.target, step->data.camera_move_to.args.instant, step->data.camera_move_to.args.move_target);
            break;
        }
        case CUTSCENE_STEP_CAMERA_WAIT:
            break;
        case CUTSCENE_STEP_INTERACT_WITH_LOCATION: {
            struct cutscene_actor* subject = cutscene_runner_lookup_actor(cutscene, evaluation_context_pop(&cutscene->context));
            if (!subject) {
                break;
            }

            struct named_location* target = scene_find_location(step->data.interact_with_location.location_name);
            if (!target) {
                break;
            }

            cutscene_actor_interact_with(
                subject,
                step->data.interact_with_location.type,
                &target->position
            );
            break;
        }
        case CUTSCENE_STEP_FADE:
            fade_effect_set(fade_colors[step->data.fade.color], step->data.fade.duration);
            break;
        case CUTSCENE_STEP_INTERACT_WITH_POSITION: {
            struct cutscene_actor* subject = cutscene_runner_lookup_actor(cutscene, evaluation_context_pop(&cutscene->context));
            if (!subject) {
                break;
            }

            cutscene_actor_interact_with(
                subject,
                step->data.interact_with_location.type,
                &step->data.interact_with_position.position
            );
            break;
        }
        case CUTSCENE_STEP_NPC_WAIT: {
            cutscene_runner.active_step_data.npc_wait.target = evaluation_context_pop(&cutscene->context);
            break;
        }
        case CUTSCENE_STEP_NPC_SET_SPEED: {
            struct cutscene_actor* subject = cutscene_runner_lookup_actor(cutscene, evaluation_context_pop(&cutscene->context));
            if (!subject) {
                break;
            }
            cutscene_actor_set_speed(subject, step->data.npc_set_speed.speed);
            break;
        }
        case CUTSCENE_STEP_SHOW_TITLE: {
            area_title_show(step->data.show_title.message);
            break;
        }
        case CUTSCENE_STEP_SHOW_BOSS_HEALTH: {
            hud_show_boss_health(&current_scene->hud, step->data.show_boss_health.name, cutscene_runner_translate_entity(cutscene, evaluation_context_pop(&cutscene->context)));
            break;
        }
        case CUTSCENE_STEP_LOOK_AT_SUBJECT: {
            struct dynamic_object* subject = collision_scene_find_object(cutscene->subject);
            if (!subject) {
                break;
            }
            cutscene_actor_interact_with(&current_scene->player.cutscene_actor, INTERACTION_LOOK_SPACE, subject->position);
            break;
        }
        case CUTSCENE_STEP_NPC_ANIMATE: {
            entity_id subject_id = evaluation_context_pop(&cutscene->context);
            cutscene_actor_t* subject = cutscene_runner_lookup_actor(cutscene, subject_id);
            if (!subject) {
                break;
            }
            cutscene_actor_run_animation(subject, step->data.npc_animate.animation_name, step->data.npc_animate.loop);
            break;
        }
        case CUTSCENE_STEP_PRINT: {
            int args[step->data.print.message.nargs];
            evaluation_context_popn(&cutscene->context, args, step->data.print.message.nargs);
            char message[128];
            dialog_box_format_string(message, step->data.print.message.template, args);
            debugf("%s\n", message);
            break;
        }
        case CUTSCENE_STEP_SPAWN: {
            int spawner = evaluation_context_pop(&cutscene->context);
            scene_spawn_entity(current_scene, spawner >> 16, spawner & 0xFFFF);
            break;
        }
        case CUTSCENE_STEP_CALLBACK: {
            step->data.callback.callback(step->data.callback.data);
            break;
        }
        case CUTSCENE_STEP_LOAD_SCENE: {
            scene_queue_next(step->data.load_scene.scene);
            break;
        }
        case CUTSCENE_STEP_DESPAWN: {
            entity_despawn(evaluation_context_pop(&cutscene->context));
            break;
        }
        case CUTSCENE_STEP_START_TIMER: {
            cutscene_timer_start(step->data.start_timer.time, step->data.start_timer.cutscene);
            break;
        }
        case CUTSCENE_STEP_CANCEL_TIMER: {
            cutscene_timer_cancel();
            break;
        }
        case CUTSCENE_STEP_STOPWATCH_SHOW: {
            cutscene_stopwatch_set_active(step->data.stopwatch.value);
            break;
        }
        case CUTSCENE_STEP_STOPWATCH_RUN: {
            cutscene_stopwatch_set_running(step->data.stopwatch.value);
            break;
        }
        case CUTSCENE_STEP_AUDIO_PAUSE:
            if (step->data.audio_pause.is_paused) {
                audio_pause_all();
            } else {
                audio_unpause_all();
            }
            break;
        case CUTSCENE_STEP_SHOW_IMAGE: {
            if (!step->data.show_image.filename || !*step->data.show_image.filename) {
                image_overlay_set(NULL);
            } else {
                image_overlay_set(step->data.show_image.filename);
            }
            break;
        }
        case CUTSCENE_STEP_TEMPLATE_STRING: {
            int args[step->data.print.message.nargs];
            evaluation_context_popn(&cutscene->context, args, step->data.template_string.message.nargs);
            int str_length = dialog_box_format_string(cutscene->current_string_start, step->data.template_string.message.template, args);
            evaluation_context_push(&cutscene->context, (int)cutscene->current_string_start);
            cutscene->current_string_start += str_length;
            assert(cutscene->current_string_start <= &cutscene->string_stack[MAX_STRING_STACK_SIZE]);
        }
        case CUTSCENE_STEP_FUNCTION_CALL:
            // logic is done in update step
            break;
        case CUTSCENE_STEP_COUNT:
            assert(false);
            break;
    }
}

bool cutscene_runner_update_step(struct cutscene_active_entry* active_entry, struct cutscene_step* step) {
    switch (step->type)
    {
        case CUTSCENE_STEP_DIALOG:
        case CUTSCENE_STEP_ASK:
            if (!cutscene_runner.active_step_data.dialog.has_shown && !dialog_box_is_active()) {
                int args[step->data.dialog.message.nargs];
                evaluation_context_popn(&active_entry->context, args, step->data.dialog.message.nargs);
                if (step->type == CUTSCENE_STEP_DIALOG) {
                    dialog_box_show(step->data.dialog.message.template, args, NULL, NULL);
                } else {
                    dialog_box_ask(step->data.dialog.message.template, args, NULL, NULL);
                }
                cutscene_runner.active_step_data.dialog.has_shown = true;
            } else if (cutscene_runner.active_step_data.dialog.has_shown) {
                return !dialog_box_is_active();
            }
            return false;
        case CUTSCENE_STEP_SHOW_ITEM:
            return show_item_update(&cutscene_runner.show_item, &step->data);
        case CUTSCENE_STEP_JUMP_IF_NOT:
            if (!evaluation_context_pop(&active_entry->context)) {
                CUTSCENE_CURR_FRAME(active_entry)->current_instruction += step->data.jump.offset;
            }
            return true;
        case CUTSCENE_STEP_JUMP:
            CUTSCENE_CURR_FRAME(active_entry)->current_instruction += step->data.jump.offset;
            return true;
        case CUTSCENE_STEP_DELAY:
            cutscene_runner.active_step_data.delay.time -= fixed_time_step;
            return cutscene_runner.active_step_data.delay.time <= 0.0f;
        case CUTSCENE_STEP_CAMERA_WAIT:
            return !camera_is_animating(&current_scene->camera_controller);
        case CUTSCENE_STEP_NPC_WAIT: {
            struct cutscene_actor* subject = cutscene_runner_lookup_actor(active_entry, cutscene_runner.active_step_data.npc_wait.target);
            return !subject || !cutscene_actor_is_moving(subject);
        }
        case CUTSCENE_STEP_FUNCTION_CALL: {
            cutscene_stack_entry_t* curr_frame = CUTSCENE_CURR_FRAME(active_entry);
            cutscene_t* cutscene = curr_frame->cutscene;
            int next_fn = step->data.function_call.fn_index;
            assert(next_fn >= 0 && next_fn < cutscene->function_count);

            ++active_entry->current_depth;

            assert(active_entry->current_depth < MAX_CALL_STACK_SIZE);

            cutscene_function_t* fn = &cutscene->functions[next_fn];

            assert(fn->arg_c == step->data.function_call.argc);
            
            *CUTSCENE_CURR_FRAME(active_entry) = (cutscene_stack_entry_t){
                .current_instruction = -1,
                .cutscene = cutscene,
                .function = fn,
                .string_stack_position = 0,
                .stack_position = evaluation_context_stack_size(&active_entry->context),
                .retc = step->data.function_call.retc,
            };
        }
        default:
            return true;
    }
}

void cutscene_runner_cancel_step(struct cutscene_active_entry* cutscene, struct cutscene_step* step) {
    switch (step->type) {
        case CUTSCENE_STEP_DIALOG:
        case CUTSCENE_STEP_ASK:
            dialog_box_hide();
            return;
        default:
            return;
    }
}

void cutscene_runner_start(struct cutscene* cutscene, int function_index, cutscene_finish_callback finish_callback, void* data, entity_id subject) {
    if (function_index < 0 || function_index >= cutscene->function_count || cutscene->functions[function_index].step_count == 0) {
        if (finish_callback) {
            finish_callback(cutscene, data, NULL);
        }
        return;
    }

    cutscene_function_t* fn = &cutscene->functions[function_index];

    if (cutscene->step_count == 0) {
        if (finish_callback) {
            finish_callback(cutscene, data, NULL);
        }
        return;
    }

    struct cutscene_active_entry* next = &cutscene_runner.active_cutscene;
    next->finish_callback = finish_callback;
    next->data = data;
    next->subject = subject;
    next->current_string_start = &next->string_stack[0];
    next->current_depth = 0;
    *CUTSCENE_CURR_FRAME(next) = (cutscene_stack_entry_t){
        .current_instruction = 0,
        .cutscene = cutscene,
        .function = fn,
        .string_stack_position = next->current_string_start - next->string_stack,
        .stack_position = 0,
        .retc = fn->return_count,
    };
    evaluation_context_init(&next->context);

    cutscene_runner_init_step(next, &fn->steps[0]);
}

void cutscene_runner_check_queue() {
    struct cutscene_queue_entry* queue_entry = &cutscene_runner.queue[cutscene_runner.next_cutscene];

    if (!queue_entry->cutscene) {
        return;
    }

    cutscene_runner.next_cutscene = cutscene_next_queue_index(cutscene_runner.next_cutscene);

    cutscene_runner_start(queue_entry->cutscene, queue_entry->function_index, queue_entry->finish_callback, queue_entry->data, queue_entry->subject);
    queue_entry->cutscene = NULL;
}

void cutscene_runner_pop_call(cutscene_active_entry_t* entry, cutscene_stack_entry_t* prev) {
    int retc = prev->function->return_count;

    int current_size = evaluation_context_stack_size(&entry->context);
    int pop_count = current_size - prev->stack_position;

    assert(pop_count >= retc);

    int result[retc];
    evaluation_context_popn(&entry->context, result, retc);
    
    pop_count -= retc;
    if (pop_count >= 0) {
        evaluation_context_popn(&entry->context, NULL, pop_count);
    }

    for (int i = 0; i < retc && i < prev->retc; i += 1) {
        evaluation_context_push(&entry->context, result[i]);
    }

    for (int i = retc; i < prev->retc; i += 1) {
        evaluation_context_push(&entry->context, 0);
    }
}

void cutscene_runner_step_instruction() {
    struct cutscene_active_entry* active_cutscene = &cutscene_runner.active_cutscene;

    while (CUTSCENE_IS_RUNNING(active_cutscene)) {
        cutscene_stack_entry_t* entry = CUTSCENE_CURR_FRAME(active_cutscene);
        entry->current_instruction += 1;
        
        if (entry->current_instruction < entry->function->step_count) {
            struct cutscene_step* step = &entry->function->steps[entry->current_instruction];
            cutscene_runner_init_step(active_cutscene, step);
            return;
        }

        cutscene_runner_pop_call(active_cutscene, entry);
        --active_cutscene->current_depth;

        if (active_cutscene->current_depth >= 0) {
            continue;
        }

        if (active_cutscene->finish_callback) {
            active_cutscene->finish_callback(entry->cutscene, active_cutscene->data, &active_cutscene->context);
        }

        evaluation_context_destroy(&active_cutscene->context);
    }

    cutscene_runner_check_queue();
}

#define MAX_SCRIPT_TIME             5000
#define MAX_INSTRUCTIONS_PER_FRAME  20

void cutscene_runner_update(void* data) {
    uint64_t start_time = get_ticks_us();
    int i = 0;

    while (cutscene_runner_is_running() && get_ticks_us() - start_time < MAX_SCRIPT_TIME && i < MAX_INSTRUCTIONS_PER_FRAME) {
        struct cutscene_active_entry* active_cutscene = &cutscene_runner.active_cutscene;
        cutscene_stack_entry_t* entry = CUTSCENE_CURR_FRAME(active_cutscene);
        struct cutscene_step* step = &entry->function->steps[entry->current_instruction];
    
        if (!cutscene_runner_update_step(active_cutscene, step)) {
            return;
        }
        cutscene_runner_step_instruction();
        i += 1;
    }

}

void cutscene_runner_render(void* data) {
    show_item_render(&cutscene_runner.show_item);
}

void cutscene_runner_init() {
    cutscene_runner.next_cutscene = 0;
    cutscene_runner.last_cutscene = 0;
    cutscene_runner.active_cutscene.current_depth = -1;

    show_item_init(&cutscene_runner.show_item);
    // update_remove() is never called
    update_add(&cutscene_runner, cutscene_runner_update, 0, UPDATE_LAYER_WORLD | UPDATE_LAYER_DIALOG | UPDATE_LAYER_PAUSE_MENU);
    menu_add_callback(cutscene_runner_render, &cutscene_runner, MENU_PRIORITY_HUD);

    for (int i = 0; i < MAX_QUEUED_CUTSCENES; i += 1) {
        cutscene_runner.queue[i].cutscene = NULL;
    }
}

void cutscene_runner_enqueue(struct cutscene* cutscene, int function_index, cutscene_finish_callback finish_callback, void* data, entity_id subject) {
    int next_cutscene = cutscene_runner.last_cutscene;

    cutscene_runner.last_cutscene = cutscene_next_queue_index(cutscene_runner.last_cutscene);

    struct cutscene_queue_entry* queue_entry = &cutscene_runner.queue[next_cutscene];

    assert(!queue_entry->cutscene);

    queue_entry->cutscene = cutscene;
    queue_entry->function_index = function_index;
    queue_entry->finish_callback = finish_callback;
    queue_entry->data = data;
    queue_entry->subject = subject;
}

void cutscene_runner_run(struct cutscene* cutscene, int function_index, cutscene_finish_callback finish_callback, void* data, entity_id subject) {
    if (CUTSCENE_IS_RUNNING(&cutscene_runner.active_cutscene)) {
        cutscene_runner_enqueue(cutscene, function_index, finish_callback, data, subject);
    } else {
        cutscene_runner_start(cutscene, function_index, finish_callback, data, subject);
    }
}

bool cutscene_runner_is_running() {
    return CUTSCENE_IS_RUNNING(&cutscene_runner.active_cutscene);
}

void cutscene_cancel_active(struct cutscene_active_entry* active_entry) {
    cutscene_stack_entry_t* entry = CUTSCENE_CURR_FRAME(active_entry);
    cutscene_runner_cancel_step(active_entry, &entry->function->steps[entry->current_instruction]);
    active_entry->current_depth = -1;
    if (active_entry->finish_callback) {
        active_entry->finish_callback(active_entry->call_stack[0].cutscene, active_entry->data, NULL);
    }
}

void cutscene_runner_cancel_from_queue(struct cutscene* cutscene) {
    int curr = cutscene_runner.next_cutscene;
    int write_index = curr;
    int deletion_count = 0;

    for (int i = 0; i < MAX_QUEUED_CUTSCENES; i += 1) {
        struct cutscene_queue_entry* entry = &cutscene_runner.queue[curr];
        
        if (curr != write_index) {
            cutscene_runner.queue[write_index] = cutscene_runner.queue[curr];
        }

        if (entry->cutscene == cutscene) {
            if (entry->finish_callback) {
                entry->finish_callback(entry->cutscene, entry->data, NULL);
            }

            entry->cutscene = NULL;
            if (curr == cutscene_runner.next_cutscene) {
                cutscene_runner.next_cutscene = cutscene_next_queue_index(cutscene_runner.next_cutscene);
                write_index = cutscene_runner.next_cutscene;
            } else {
                deletion_count += 1;
            }
        } else {
            write_index = cutscene_next_queue_index(write_index);
        }

        curr = cutscene_next_queue_index(curr);
    }

    for (int i = 0; i < deletion_count; i += 1) {
        cutscene_runner.queue[write_index].cutscene = NULL;
        write_index = cutscene_next_queue_index(write_index);
        cutscene_runner.last_cutscene = cutscene_prev_queue_index(cutscene_runner.last_cutscene);
    }
}

void cutscene_runner_cancel(struct cutscene* cutscene) {
    cutscene_runner_cancel_from_queue(cutscene);
    
    cutscene_active_entry_t* active_entry = &cutscene_runner.active_cutscene;

    if (CUTSCENE_IS_RUNNING(active_entry) && active_entry->call_stack[0].cutscene == cutscene) {
        cutscene_cancel_active(active_entry);
        cutscene_runner_check_queue();
    }
}

void _cutscene_runner_free_on_finish(struct cutscene* cutscene, void* data, evaluation_context_t* context) {
    cutscene_free(cutscene);
}

// used to free a cutscene created by cutscene_new() or cutscene_load()
cutscene_finish_callback cutscene_runner_free_on_finish() {
    return _cutscene_runner_free_on_finish;
}