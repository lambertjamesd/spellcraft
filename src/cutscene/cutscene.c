#include "cutscene.h"

#include <libdragon.h>
#include <malloc.h>
#include <assert.h>

#include "../time/time.h"

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

// release with cutscene_free()
struct cutscene* cutscene_load(char* filename) {
    FILE* file = asset_fopen(filename, NULL);

    int header;
    fread(&header, 1, 4, file);
    assert(header == EXPECTED_HEADER);

    uint16_t step_count;
    fread(&step_count, 2, 1, file);

    uint16_t locals_size;
    fread(&locals_size, 2, 1, file);
    
    // release with cutscene_free()
    struct cutscene* result = cutscene_new(step_count, locals_size);

    if (locals_size) {
        fread(result->locals, 1, locals_size, file);
    }

    for (int i = 0; i < step_count; i += 1) {
        struct cutscene_step* step = &result->steps[i];

        uint8_t step_type;
        fread(&step_type, 1, 1, file);

        step->type = step_type;

        switch (step_type) {
            case CUTSCENE_STEP_DIALOG:
                cutscene_load_template_string(&step->data.dialog.message, file);
                break;
            case CUTSCENE_STEP_PAUSE:
                fread(&step->data.pause, 1, 2, file);
                step->data.pause.layers = UPDATE_LAYER_WORLD;
                break;
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
            case CUTSCENE_STEP_SET_LOCAL:
            case CUTSCENE_STEP_SET_GLOBAL:
                fread(&step->data.store_variable, 4, 1, file);
                break;
            case CUTSCENE_STEP_LOOK_AT_NPC:
            case CUTSCENE_STEP_MOVE_TO_NPC:
                fread(&step->data.interact_with_npc.subject, 4, 1, file);
                fread(&step->data.interact_with_npc.target, 4, 1, file);
                fread(&step->data.interact_with_npc.speed, 4, 1, file);
                break;
            case CUTSCENE_STEP_IDLE_NPC:
                fread(&step->data.interact_with_npc.subject, 4, 1, file);
                step->data.interact_with_npc.target.unique_id = 0;
                break;
        }
    }

    fclose(file);
    
    return result;
}

void cutscene_init(struct cutscene* cutscene, int capacity, int locals_size) {
    cutscene->steps = malloc(sizeof(struct cutscene_step) * capacity);
    cutscene->step_count = capacity;
    cutscene->locals_size = locals_size;
    if (locals_size) {
        cutscene->locals = malloc(locals_size);
    } else {
        cutscene->locals = NULL;
    }
}

void cutscene_destroy(struct cutscene* cutscene) {
    for (int i = 0; i < cutscene->step_count; i += 1) {
        struct cutscene_step* step = &cutscene->steps[i];

        switch (step->type) {
            case CUTSCENE_STEP_DIALOG:
                cutscene_destroy_template_string(&step->data.dialog.message);
                break;
            case CUTSCENE_STEP_EXPRESSION:
                expression_destroy(&step->data.expression.expression);
                break;
            default:
                break;
        }
    }

    free(cutscene->steps);
    free(cutscene->locals);
}

struct cutscene* cutscene_new(int capacity, int locals_capacity) {
    struct cutscene* result = malloc(sizeof(struct cutscene));
    cutscene_init(result, capacity, locals_capacity);
    return result;
}

void cutscene_free(struct cutscene* cutscene) {
    if (!cutscene) {
        return;
    }
    cutscene_destroy(cutscene);
    free(cutscene);
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

void cutscene_builder_pause(struct cutscene_builder* builder, bool should_pause, bool should_change_game_mode, int layers) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);

    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_PAUSE,
        .data = {
            .pause = {
                .should_pause = should_pause,
                .should_change_game_mode = should_change_game_mode,
                .layers = layers,
            },
        },
    };
}

void cutscene_builder_dialog(struct cutscene_builder* builder, char* message) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);

    char* message_copy = malloc(strlen(message) + 1);
    // free(message_copy) is done in cutscene_load_template_string
    strcpy(message_copy, message);

    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_DIALOG,
        .data = {
            .dialog = {
                .message = {
                    .template = message_copy,
                    .nargs = 0,
                },
            },
        },
    };
}

void cutscene_builder_show_item(struct cutscene_builder* builder, enum inventory_item_type item, bool should_show) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);
    
    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_SHOW_ITEM,
        .data = {
            .show_item = {
                .item = item,
                .should_show = should_show,
            },
        },
    };
}

void cutscene_builder_delay(struct cutscene_builder* builder, float delay) {
    struct cutscene_step* step = cutscene_builder_next_step(builder);
    
    *step = (struct cutscene_step){
        .type = CUTSCENE_STEP_DELAY,
        .data = {
            .delay = {
                .duration = delay,
            },
        },
    };
}

// release with cutscene_free()
struct cutscene* cutscene_builder_finish(struct cutscene_builder* builder) {
    // release with cutscene_free()
    struct cutscene* result = cutscene_new(builder->step_count, 0);
    memcpy(result->steps, builder->steps, sizeof(struct cutscene_step) * builder->step_count);
    return result;
}
