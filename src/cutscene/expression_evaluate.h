#ifndef __CUTSCENE_EXPRESSION_EVALUATE_H__
#define __CUTSCENE_EXPRESSION_EVALUATE_H__

#include "expression.h"
#include "evaluation_context.h"

// the evaluation result is left on the top of th context stack
void expression_evaluate(struct evaluation_context* context, struct expression* expression);

#endif