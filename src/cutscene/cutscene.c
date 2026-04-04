#include "cutscene.h"

#include <libdragon.h>
#include <malloc.h>
#include <assert.h>

#include "../time/time.h"
#include "evaluation_context.h"
#include "cutscene_step_fn.h"

#define EXPECTED_HEADER 0x4354534E

char* cutscene_load_string(FILE* file) {
    uint8_t lower_length;
    fread(&lower_length, 1, 1, file);

    uint16_t length = 0;

    if (lower_length & 0x80) {
        uint8_t upper_length;
        fread(&upper_length, 1, 1, file);

        length = ((uint16_t)upper_length << 7) | (lower_length & 0x7F);
    } else {
        length = lower_length;
    }

    char* result = malloc(length + 1);
    fread(result, 1, length, file);
    result[length] = '\0';
    return result;
}

void cutscene_load_template_string(struct templated_string* string, FILE* file) {
    uint8_t nargs;
    fread(&nargs, 1, 1, file);
    string->nargs = nargs;
    string->template = cutscene_load_string(file);
}

void cutscene_destroy_template_string(struct templated_string* string) {
    free(string->template);
}

char* string_load(FILE* file) {
    uint8_t result_length;
    fread(&result_length, 1, 1, file);

    if (result_length == 0) {
        return NULL;
    }

    char* result = malloc(result_length + 1);
    fread(result, 1, result_length, file);
    result[result_length] = '\0';
    return result;
}

// release with cutscene_free()
struct cutscene* cutscene_load(const char* filename) {
    FILE* file = asset_fopen(filename, NULL);

    int header;
    fread(&header, 1, 4, file);
    assert(header == EXPECTED_HEADER);

    uint16_t step_count;
    fread(&step_count, 2, 1, file);

    uint16_t function_count;
    fread(&function_count, 2, 1, file);

    // release with cutscene_free()
    struct cutscene* result = cutscene_new(step_count, function_count);

    for (int i = 0; i < step_count; i += 1) {
        struct cutscene_step* step = &result->steps[i];

        uint8_t step_type;
        fread(&step_type, 1, 1, file);

        step->type = step_type;

        switch (step_type) {
            case CUTSCENE_STEP_EXPRESSION:
                expression_load(&step->data.expression.expression, file);
                break;
            case CUTSCENE_STEP_JUMP_IF_NOT:
            case CUTSCENE_STEP_JUMP: {
                int16_t offset;
                fread(&offset, 2, 1, file);
                step->data.jump.offset = offset;
                break;
            }
            case CUTSCENE_STEP_SET_SCENE:
            case CUTSCENE_STEP_SET_GLOBAL:
                fread(&step->data.store_variable, 4, 1, file);
                break;
            case CUTSCENE_STEP_TEMPLATE_STRING:
                cutscene_load_template_string(&step->data.template_string.message, file);
                break;
            case CUTSCENE_STEP_FUNCTION_CALL:
            case CUTSCENE_STEP_BUILT_IN_FN:
                fread(&step->data.function_call.fn_index, 2, 1, file);
                fread(&step->data.function_call.argc, 1, 1, file);
                fread(&step->data.function_call.retc, 1, 1, file);
                break;
            default:
                assert(step_type < CUTSCENE_STEP_COUNT);
                break;
        }
    }
    
    cutscene_step_t* function_steps = result->steps;
    cutscene_function_t* fn = result->functions;
    for (int i = 0; i < function_count; i += 1, fn += 1) {
        uint16_t function_size;
        fread(&function_size, 2, 1, file);
        fread(&fn->arg_c, 1, 1, file);
        fread(&fn->return_count, 1, 1, file);

        fn->steps = function_steps;
        fn->step_count = function_size;
        fn->name = string_load(file);

        function_steps += function_size;
    }

    fclose(file);
    
    return result;
}

void cutscene_init(struct cutscene* cutscene, int capacity, int function_count) {
    cutscene->steps = malloc(sizeof(struct cutscene_step) * capacity);
    cutscene->functions = malloc(sizeof(cutscene_function_t) * function_count);
    cutscene->step_count = capacity;
    cutscene->function_count = function_count;
}

void cutscene_destroy(struct cutscene* cutscene) {
    for (int i = 0; i < cutscene->step_count; i += 1) {
        struct cutscene_step* step = &cutscene->steps[i];

        switch (step->type) {
            case CUTSCENE_STEP_EXPRESSION:
                expression_destroy(&step->data.expression.expression);
                break;
            case CUTSCENE_STEP_TEMPLATE_STRING:
                cutscene_destroy_template_string(&step->data.template_string.message);
                break;
            default:
                break;
        }
    }

    for (int i = 0; i < cutscene->function_count; i += 1) {
        free(cutscene->functions[i].name);
    }

    free(cutscene->steps);
    free(cutscene->functions);
}

struct cutscene* cutscene_new(int capacity, int function_count) {
    struct cutscene* result = malloc(sizeof(struct cutscene));
    cutscene_init(result, capacity, function_count);
    return result;
}

void cutscene_free(struct cutscene* cutscene) {
    if (!cutscene) {
        return;
    }
    cutscene_destroy(cutscene);
    free(cutscene);
}

int cutscene_find_function_index(struct cutscene* cutscene, const char* name) {
    for (int i = 0; i < cutscene->function_count; i += 1) {
        const char* fn_name = cutscene->functions[i].name;
        if ((!fn_name && !name) || (fn_name && strcmp(fn_name, name) == 0)) {
            return i;
        }
    }

    return -1;
}

void cutscene_builder_init(struct cutscene_builder* builder) {
    builder->step_count = 0;
}

struct cutscene_step* cutscene_builder_next_step(struct cutscene_builder* builder) {
    assert(builder->step_count < MAX_BUILDER_STEP_COUNT);

    struct cutscene_step* result = &builder->steps[builder->step_count];
    builder->step_count += 1;
    return result;
}

void cutscene_builder_message(struct cutscene_builder* builder, const char* message) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);

    int len = strlen(message);
    char* template = malloc(len + 1);
    strcpy(template, message);

    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_TEMPLATE_STRING,
        .data = {
            .template_string = {
                .message = {
                    .template = template,
                    .nargs = 0,
                }
            },
        },
    };
}

void cutscene_builder_call_function(struct cutscene_builder* builder, enum cutscene_step_fn_index fn, int argc, int retc) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);
    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_BUILT_IN_FN,
        .data = {
            .function_call = {
                .fn_index = fn,
                .argc = argc,
                .retc = retc,
            },
        },
    };
}

void cutscene_builder_pause(struct cutscene_builder* builder, bool should_pause, bool should_change_game_mode) {
    if (should_change_game_mode) {
        cutscene_builder_call_function(builder, should_pause ? CUTSCENE_FN_PAUSE : CUTSCENE_FN_UNPAUSE, 0, 0);
    } else {
        cutscene_builder_call_function(builder, should_pause ? CUTSCENE_FN_ENTER_MENU : CUTSCENE_FN_EXIT_MENU, 0, 0);
    }
}

void cutscene_builder_dialog(struct cutscene_builder* builder, const char* message) {
    cutscene_builder_message(builder, message);
    cutscene_builder_call_function(builder, CUTSCENE_FN_SAY, 0, 0);
}

void cutscene_builder_show_item(struct cutscene_builder* builder, enum inventory_item_type item, bool should_show) {
    expression_builder_t expr;
    expression_builder_init(&expr);
    expression_builder_load_literal(&expr, item);
    expression_builder_load_literal(&expr, should_show);

    cutscene_builder_expression(builder, &expr);

    cutscene_builder_call_function(builder, CUTSCENE_FN_SHOW_ITEM, 2, 0);
}

void cutscene_builder_delay(struct cutscene_builder* builder, float delay) {
    expression_builder_t expr;
    expression_builder_init(&expr);
    expression_builder_load_float(&expr, delay);

    cutscene_builder_expression(builder, &expr);
    
    cutscene_builder_call_function(builder, CUTSCENE_FN_DELAY, 1, 0);
}

void cutscene_builder_interact_npc(
    struct cutscene_builder* builder,
    enum interaction_type type,
    entity_id subject,
    entity_id target
) {
    expression_builder_t expr;
    expression_builder_init(&expr);
    expression_builder_load_literal(&expr, type);
    expression_builder_load_literal(&expr, subject);
    expression_builder_load_literal(&expr, target);
    cutscene_builder_expression(builder, &expr);

    
    cutscene_builder_call_function(builder, CUTSCENE_FN_INTERACT_NPC, 3, 0);
}

void cutscene_builder_npc_set_speed(struct cutscene_builder* builder, entity_id subject, float speed) {
    expression_builder_t expr;
    expression_builder_init(&expr);
    expression_builder_load_literal(&expr, subject);
    expression_builder_load_float(&expr, speed);
    cutscene_builder_expression(builder, &expr);

    cutscene_builder_call_function(builder, CUTSCENE_FN_NPC_SET_SPEED, 2, 0);
}

void cutscene_builder_interact_position(
    struct cutscene_builder* builder,
    enum interaction_type type,
    entity_id subject,
    struct Vector3* position
) {
    expression_builder_t expr;
    expression_builder_init(&expr);
    expression_builder_load_literal(&expr, type);
    expression_builder_load_literal(&expr, subject);
    expression_builder_load_float(&expr, position->x);
    expression_builder_load_float(&expr, position->y);
    expression_builder_load_float(&expr, position->z);
    cutscene_builder_expression(builder, &expr);

    cutscene_builder_call_function(builder, CUTSCENE_FN_INTERACT_POSITION, 5, 0);
}

void cutscene_builder_npc_wait(
    struct cutscene_builder* builder,
    entity_id subject
) {    
    expression_builder_t expr;
    expression_builder_init(&expr);
    expression_builder_load_literal(&expr, subject);
    cutscene_builder_expression(builder, &expr);

    cutscene_builder_call_function(builder, CUTSCENE_FN_NPC_WAIT, 1, 0);
}

void cutscene_builder_camera_wait(struct cutscene_builder* builder) {
    cutscene_builder_call_function(builder, CUTSCENE_FN_CAMERA_WAIT, 0, 0);
}

void cutscene_builder_camera_follow(struct cutscene_builder* builder) {
    cutscene_builder_call_function(builder, CUTSCENE_FN_CAMERA_FOLLOW, 0, 0);
}

void cutscene_builder_camera_return(struct cutscene_builder* builder) {
    cutscene_builder_call_function(builder, CUTSCENE_FN_CAMERA_RETURN, 0, 0);
}

void cutscene_builder_camera_look_at(struct cutscene_builder* builder, entity_id target) {
    expression_builder_t expr;
    expression_builder_init(&expr);
    expression_builder_load_literal(&expr, target);
    cutscene_builder_expression(builder, &expr);
    
    cutscene_builder_call_function(builder, CUTSCENE_FN_CAMERA_LOOK_AT_NPC, 1, 0);
}

void cutscene_builder_camera_move_to(struct cutscene_builder* builder, struct Vector3* position, bool instant) {
    expression_builder_t expr;
    expression_builder_init(&expr);
    expression_builder_load_float(&expr, position->x);
    expression_builder_load_float(&expr, position->y);
    expression_builder_load_float(&expr, position->z);
    expression_builder_load_literal(&expr, instant);
    cutscene_builder_expression(builder, &expr);

    cutscene_builder_call_function(builder, CUTSCENE_FN_CAMERA_MOVE_TO, 4, 0);
}

void cutscene_builder_camera_look_at_pos(struct cutscene_builder* builder, struct Vector3* position, bool instant) {
    expression_builder_t expr;
    expression_builder_init(&expr);
    expression_builder_load_float(&expr, position->x);
    expression_builder_load_float(&expr, position->y);
    expression_builder_load_float(&expr, position->z);
    expression_builder_load_literal(&expr, instant);
    cutscene_builder_expression(builder, &expr);

    cutscene_builder_call_function(builder, CUTSCENE_FN_CAMERA_LOOK_AT_POS, 4, 0);
}

void cutscene_builder_set_boolean(struct cutscene_builder* builder, boolean_variable variable, bool value) {
    struct cutscene_step* expression = cutscene_builder_next_step(builder);
    struct cutscene_step* set = cutscene_builder_next_step(builder);

    *expression = (struct cutscene_step){
        .type = CUTSCENE_STEP_EXPRESSION,
    };
    expression_load_literal(&expression->data.expression.expression, value);

    if (SCENE_VARIABLE_FLAG & variable) {
        *set = (struct cutscene_step){
            .type = CUTSCENE_STEP_SET_SCENE,
            .data = {
                .store_variable = {
                    .data_type = DATA_TYPE_BOOL,
                    .word_offset = variable ^ SCENE_VARIABLE_FLAG,
                },
            },
        };
    } else {
        *set = (struct cutscene_step){
            .type = CUTSCENE_STEP_SET_GLOBAL,
            .data = {
                .store_variable = {
                    .data_type = DATA_TYPE_BOOL,
                    .word_offset = variable,
                },
            },
        };
    }
}

void cutscene_builder_callback(struct cutscene_builder* builder, cutscene_step_callback callback, void* data) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);

    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_CALLBACK,
        .data.callback = {
            .callback = callback,
            .data = data,
        },
    };
}

void cutscene_builder_expression(struct cutscene_builder* builder, expression_builder_t* expression) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);
    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_EXPRESSION,
    };
    expression_builder_finish(expression, &step->data.expression.expression);
}

void cutscene_builder_load_scene(struct cutscene_builder* builder, const char* scene) {
    cutscene_builder_message(builder, scene);
    cutscene_builder_call_function(builder, CUTSCENE_FN_LOAD_SCENE, 1, 0);
}

void cutscene_builder_fade(struct cutscene_builder* builder, enum fade_colors color, float duration) {
    expression_builder_t expr;
    expression_builder_init(&expr);
    expression_builder_load_literal(&expr, color);
    expression_builder_load_float(&expr, duration);
    cutscene_builder_expression(builder, &expr);

    cutscene_builder_call_function(builder, CUTSCENE_FN_LOAD_FADE, 2, 0);
}

// release with cutscene_free()
struct cutscene* cutscene_builder_finish(struct cutscene_builder* builder) {
    // release with cutscene_free()
    struct cutscene* result = cutscene_new(builder->step_count, 1);
    memcpy(result->steps, builder->steps, sizeof(struct cutscene_step) * builder->step_count);
    result->functions[0] = (cutscene_function_t){
        .name = NULL,
        .step_count = builder->step_count,
        .steps = result->steps,
    };
    return result;
}

