#include "cutscene_runner_context.h"

void cutscene_context_init(cutscene_runner_context_t* context, entity_id subject) {
    context->subject_id = subject;
    evaluation_context_init(&context->eval);
    context->current_string_start = context->string_stack;
}