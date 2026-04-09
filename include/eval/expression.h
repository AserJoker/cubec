#ifndef _H_CUBEC_EVAL_EXPRESSION_
#define _H_CUBEC_EVAL_EXPRESSION_
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_eval_expression(cubec_context_t ctx, cubec_ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif