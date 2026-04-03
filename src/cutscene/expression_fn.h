#ifndef __CUTSCENE_EXPRESSION_FN_H__
#define __CUTSCENE_EXPRESSION_FN_H__

#include "evaluation_context.h"

typedef int (*expression_built_in_fn)(struct evaluation_context* context, int arg_count);

expression_built_in_fn expression_lookup_fn(int type);

#endif