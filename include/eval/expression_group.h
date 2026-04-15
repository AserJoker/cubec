#ifndef _H_CUBEC_EVAL_EXPRESSION_GROUP_
#define _H_CUBEC_EVAL_EXPRESSION_GROUP_
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
value_t eval_expression_group(context_t ctx, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif