#include "expression_evaluate.h"

void expression_evaluate(struct evaluation_context* context, struct expression* expression) {
    uint8_t* current = expression->steps;
    union expression_data* data = expression->step_data;

    while (*current != EXPRESSION_TYPE_END) {
        switch (*current) {
            case EXPRESSION_TYPE_LOAD_LOCAL:
                evaluation_context_push(context, evaluation_context_load(context->local_varaibles, data->load_variable.data_type, data->load_variable.word_offset));
                ++data;
                break;
            case EXPRESSION_TYPE_LOAD_GLOBAL:
                evaluation_context_push(context, evaluation_context_load(context->global_variables, data->load_variable.data_type, data->load_variable.word_offset));
                ++data;
                break;
            case EXPRESSION_TYPE_LOAD_LITERAL:
                evaluation_context_push(context, data->literal);
                ++data;
                break;
            case EXPRESSION_TYPE_AND: {
                int a = evaluation_context_pop(context);
                int b = evaluation_context_pop(context);
                evaluation_context_push(context, a && b);
                break;
            }
            case EXPRESSION_TYPE_OR: {
                int a = evaluation_context_pop(context);
                int b = evaluation_context_pop(context);
                evaluation_context_push(context, a || b);
                break;
            }
            case EXPRESSION_TYPE_NOT:
                evaluation_context_push(
                    context,
                    !evaluation_context_pop(context)
                );
                break;
            case EXPRESSION_TYPE_EQ:
                evaluation_context_push(
                    context,
                    evaluation_context_pop(context) == evaluation_context_pop(context)
                );
                break;
            case EXPRESSION_TYPE_NEQ:
                evaluation_context_push(
                    context,
                    evaluation_context_pop(context) != evaluation_context_pop(context)
                );
                break;
            case EXPRESSION_TYPE_GT:
                evaluation_context_push(
                    context,
                    evaluation_context_pop(context) > evaluation_context_pop(context)
                );
                break;
            case EXPRESSION_TYPE_GTE:
                evaluation_context_push(
                    context,
                    evaluation_context_pop(context) >= evaluation_context_pop(context)
                );
                break;
            case EXPRESSION_TYPE_ADD:
                evaluation_context_push(
                    context,
                    evaluation_context_pop(context) + evaluation_context_pop(context)
                );
                break;
            case EXPRESSION_TYPE_SUB:
                evaluation_context_push(
                    context,
                    evaluation_context_pop(context) - evaluation_context_pop(context)
                );
                break;
            case EXPRESSION_TYPE_MUL:
                evaluation_context_push(
                    context,
                    evaluation_context_pop(context) * evaluation_context_pop(context)
                );
                break;
            case EXPRESSION_TYPE_DIV:
                evaluation_context_push(
                    context,
                    evaluation_context_pop(context) / evaluation_context_pop(context)
                );
                break;
            case EXPRESSION_TYPE_NEGATE:
                evaluation_context_push(
                    context,
                    -evaluation_context_pop(context)
                );
                break;
        }

        ++current;
    }
}
