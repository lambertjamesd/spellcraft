#ifndef __CUTSCENE_RUNNER_CONTEXT_H__
#define __CUTSCENE_RUNNER_CONTEXT_H__

#include <assert.h>
#include  "../entity/entity_id.h"
#include "../scene/scene_definition.h"
#include "evaluation_context.h"

#define MAX_STRING_STACK_SIZE   4096

struct cutscene_runner_context {
    entity_id subject_id;
    struct evaluation_context eval;
    char string_stack[MAX_STRING_STACK_SIZE];
    char* current_string_start;
};

typedef struct cutscene_runner_context cutscene_runner_context_t;

static inline entity_id cutscene_context_get_translate_entity(cutscene_runner_context_t* ctx, entity_id id) {
    if (id == ENTITY_ID_SUBJECT) {
        return ctx->subject_id;
    }

    return id;
}

static inline char* cutscene_context_peek_string(cutscene_runner_context_t* ctx) {
    return ctx->current_string_start;
}

static inline int cutscene_context_string_bytes(cutscene_runner_context_t* ctx) {
    return ctx->current_string_start - ctx->string_stack;
}

static inline char* cutscene_context_alloc_string(cutscene_runner_context_t* ctx, int len) {
    char* result = ctx->current_string_start;
    ctx->current_string_start += len;
    assert(ctx->current_string_start <= &ctx->string_stack[MAX_STRING_STACK_SIZE]);
    return result;
}

void cutscene_context_init(cutscene_runner_context_t* context, entity_id subject);

static inline void cutscene_context_destroy(cutscene_runner_context_t* context) {
    evaluation_context_destroy(&context->eval);
}

#endif