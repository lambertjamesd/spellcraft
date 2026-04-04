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
#include "cutscene_step_fn.h"

#define MAX_QUEUED_CUTSCENES    4
#define MAX_CUTSCENE_CALL_DEPTH 6

#define PACK_FN_REF(fn_index, instruction)  (((fn_index) << 16) | instruction)
#define FN_REF_GET_FN(ref)  ((ref) >> 16)
#define FN_REF_GET_INST(ref)    ((ref) & 0xFFFF)

extern struct scene* current_scene;

union cutscene_runner_data {
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
    int16_t current_depth;
    cutscene_stack_entry_t call_stack[MAX_CALL_STACK_SIZE];
    cutscene_runner_context_t context;
};

typedef struct cutscene_active_entry cutscene_active_entry_t;

#define CUTSCENE_IS_RUNNING(active_entry) ((active_entry)->current_depth != -1)
#define CUTSCENE_CURR_FRAME(active_entry)   (&(active_entry)->call_stack[(active_entry)->current_depth])

struct cutscene_runner {
    uint16_t next_cutscene;
    uint16_t last_cutscene;

    struct cutscene_queue_entry queue[MAX_QUEUED_CUTSCENES];

    cutscene_active_entry_t active_cutscene;

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

void cutscene_runner_init_step(struct cutscene_active_entry* cutscene, struct cutscene_step* step) {
    switch (step->type)
    {
        case CUTSCENE_STEP_EXPRESSION:
            expression_evaluate(&cutscene->context.eval, &step->data.expression.expression);
            break;
        case CUTSCENE_STEP_JUMP_IF_NOT:
        case CUTSCENE_STEP_JUMP:
            // logic is done in update step
            break;
        case CUTSCENE_STEP_SET_SCENE: {
            struct evaluation_context* context = &cutscene->context.eval;

            evaluation_context_save(
                expression_get_scene_variables(),
                step->data.store_variable.data_type,
                step->data.store_variable.word_offset,
                evaluation_context_pop(context)
            );
            break;
        }
        case CUTSCENE_STEP_SET_GLOBAL: {
            struct evaluation_context* context = &cutscene->context.eval;

            evaluation_context_save(
                savefile_get_globals(GLOBAL_ACCESS_MODE_WRITE),
                step->data.store_variable.data_type,
                step->data.store_variable.word_offset,
                evaluation_context_pop(context)
            );
            break;
        }
        case CUTSCENE_STEP_CALLBACK: {
            step->data.callback.callback(step->data.callback.data);
            break;
        }
        case CUTSCENE_STEP_TEMPLATE_STRING: {
            int args[step->data.print.message.nargs];
            evaluation_context_popn(&cutscene->context.eval, args, step->data.template_string.message.nargs);
            char* message = cutscene_context_peek_string(&cutscene->context);
            int str_length = dialog_box_format_string(message, step->data.template_string.message.template, args);
            cutscene_context_alloc_string(&cutscene->context, str_length);
            evaluation_context_push(&cutscene->context.eval, (int)message);

        }
        case CUTSCENE_STEP_FUNCTION_CALL:
            // logic is done in update step
            break;
        case CUTSCENE_STEP_BUILT_IN_FN: {
            cutscene_step_fn_t* fn = cutscene_step_lookup_fn(step->data.function_call.fn_index);
            cutscene_context_save_stack(&cutscene->context, step->data.function_call.argc);
            fn->init(&cutscene->context, step->data.function_call.argc);
            break;
        }
        case CUTSCENE_STEP_COUNT:
            assert(false);
            break;
    }
}

bool cutscene_runner_update_step(struct cutscene_active_entry* active_entry, struct cutscene_step* step) {
    switch (step->type)
    {
        case CUTSCENE_STEP_JUMP_IF_NOT:
            if (!evaluation_context_pop(&active_entry->context.eval)) {
                CUTSCENE_CURR_FRAME(active_entry)->current_instruction += step->data.jump.offset;
            }
            return true;
        case CUTSCENE_STEP_JUMP:
            CUTSCENE_CURR_FRAME(active_entry)->current_instruction += step->data.jump.offset;
            return true;
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
                .string_stack_position = cutscene_context_string_bytes(&active_entry->context),
                .stack_position = evaluation_context_stack_size(&active_entry->context.eval),
                .retc = step->data.function_call.retc,
            };
            return true;
        }
        case CUTSCENE_STEP_BUILT_IN_FN: {
            cutscene_step_fn_t* fn = cutscene_step_lookup_fn(step->data.function_call.fn_index);
            
            if (fn->step && !fn->step(&active_entry->context)) {
                return false;
            }

            evaluation_context_set_stack_size(&active_entry->context.eval, active_entry->context.stack_depth + step->data.function_call.retc);            
        }
        default:
            return true;
    }
}

void cutscene_runner_cancel_step(struct cutscene_active_entry* cutscene, struct cutscene_step* step) {
    switch (step->type) {
        case CUTSCENE_STEP_BUILT_IN_FN:{
            cutscene_step_fn_t* fn = cutscene_step_lookup_fn(step->data.function_call.fn_index);
            
            if (fn->cancel) {
                fn->cancel(&cutscene->context);
            }
            
            return;
        }
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
    cutscene_context_init(&next->context, subject);
    next->finish_callback = finish_callback;
    next->data = data;
    next->current_depth = 0;
    *CUTSCENE_CURR_FRAME(next) = (cutscene_stack_entry_t){
        .current_instruction = 0,
        .cutscene = cutscene,
        .function = fn,
        .string_stack_position = 0,
        .stack_position = 0,
        .retc = fn->return_count,
    };

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

    int current_size = evaluation_context_stack_size(&entry->context.eval);
    int pop_count = current_size - prev->stack_position;

    assert(pop_count >= retc);

    int result[retc];
    evaluation_context_popn(&entry->context.eval, result, retc);
    
    pop_count -= retc;
    if (pop_count >= 0) {
        evaluation_context_popn(&entry->context.eval, NULL, pop_count);
    }

    for (int i = 0; i < retc && i < prev->retc; i += 1) {
        evaluation_context_push(&entry->context.eval, result[i]);
    }

    for (int i = retc; i < prev->retc; i += 1) {
        evaluation_context_push(&entry->context.eval, 0);
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

        cutscene_context_destroy(&active_cutscene->context);
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

void cutscene_runner_init() {
    cutscene_runner.next_cutscene = 0;
    cutscene_runner.last_cutscene = 0;
    cutscene_runner.active_cutscene.current_depth = -1;

    // update_remove() is never called
    update_add(&cutscene_runner, cutscene_runner_update, 0, UPDATE_LAYER_WORLD | UPDATE_LAYER_DIALOG | UPDATE_LAYER_PAUSE_MENU);

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

void _cutscene_runner_free_on_finish(struct cutscene* cutscene, void* data, cutscene_runner_context_t* context) {
    cutscene_free(cutscene);
}

// used to free a cutscene created by cutscene_new() or cutscene_load()
cutscene_finish_callback cutscene_runner_free_on_finish() {
    return _cutscene_runner_free_on_finish;
}